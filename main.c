#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>
#include <string.h>

//************************************************
//*******************DEFINES**********************
//************************************************

#define COLOR_BORDER 10
#define COLOR_FROG 11
#define COLOR_BAD_CAR 12
#define COLOR_GOOD_CAR 13
#define COLOR_GRASS 14
#define COLOR_ROAD 15
#define COLOR_FINISH 16
#define COLOR_TEXT 17
#define COLOR_OBSTACLE 18

#define NUMBER_OF_CARS 10
#define PLAYER_DELAY 30
#define MAX_OBSTACLES 50

#define UP (position_t){ -1, 0 }
#define DOWN (position_t){ 1, 0 }
#define LEFT (position_t){ 0, -1 }
#define RIGHT (position_t){ 0, 1 }

#define MAP_WIDTH 21
#define MAP_HEIGHT 20
#define MAP_POSITION ((position_t){ 2, COLS/2 - 31/2 })

//************************************************
//*********************ENUMS**********************
//************************************************

typedef enum {
  ROAD_LR,
  ROAD_RL,
  GRASS,
  FINISH
}lane_t;


typedef enum {
  WRAPPING,
  DISAPPEARING,
}carBehaviour_t;


typedef enum {
  MOVES_RIGHT,
  MOVES_LEFT,
}carDirection_t;


typedef enum {
  PLAYING,
  LOST,
  WIN,
  STOPPED
}gameState_t;

//************************************************
//*******************STRUCTURES*******************
//************************************************

typedef struct {
  int y;
  int x;
}position_t;


typedef struct{
  position_t position;
  char borderChar;
}border_t;


typedef struct{
  position_t position;
  int width;
  int height;
  border_t border;
  lane_t *lanes;
  int roadCount;
  position_t *obstacles;
  int obstacleCount;
}map_t;


typedef struct {
  position_t position;
  int delayBetweenMoves;
  int regeneration;
}player_t;


typedef struct {
  position_t position;
  int length;
  int delayBetweenMoves;
  int regeneration;
  char good;
  char dead;
  char careful;
  carBehaviour_t behaviour;
  carDirection_t direction;
}car_t;


typedef struct {

}stork_t;


typedef struct {
  int mapWidth;
  int mapHeight;
  int playerDelay;
  int carMinDelay;
  int carMaxDelay;
  int minCarsNumber;
  int maxCarsNumber;
  int roadNumber;
}settings_t;


typedef struct {
  map_t map;
  player_t player;
  car_t* cars;
  gameState_t gameState;
  unsigned long int updates_counter;
  settings_t settings;
}game_t;



//************************************************
//*****************BASIC-FUNCTIONS****************
//************************************************


int* randomUniqueNumbers(const int min, int max, const int number) {
  max = max - min + 1;
  int *numbers = malloc(sizeof(int) * number);
  for (int iteration = 0, chosen = 0; iteration < max && chosen < number; iteration++) {
    const int remaining_numbers = max - iteration;
    const int remaining_to_choose = number - chosen;
    if ((int)random() % remaining_numbers < remaining_to_choose)
      numbers[chosen++] = iteration + min;
  }
  return numbers;
}


// int readSetting(char* parameter, FILE *file) {
//   parameter = strcat(parameter, " %d");
//   char *value;
//   fscanf(file, parameter, &value);
//   return (int)strtol(value, NULL, 10);
// }

//************************************************
//**********************INITS*********************
//************************************************

int yPositionOfRoad(const map_t map, const int n) {
  int counter = 0;
  for (int i = 0; i < map.height; i++) {
    if (map.lanes[i] == ROAD_LR || map.lanes[i] == ROAD_RL)
      counter++;
    if (counter == n)
      return i;
  }
  return -1;
}


position_t* initObstacles(map_t* map, const settings_t settings) {
  position_t *obstacles = malloc(sizeof(position_t) * MAX_OBSTACLES);
  int counter = 0;
  for (int i = 1; i < map->height-1; i++) {
    if (map->lanes[i] != GRASS)
      continue;

    int j =  (int)random()%(map->width/3) + 3;
    int *cols = randomUniqueNumbers(0, map->width - 1, j);
    for (; j>0; j--) {
      position_t obstacle;
      obstacle.y = i;
      obstacle.x = cols[j-1];
      obstacles[counter++] = obstacle;
      if (counter == MAX_OBSTACLES) {
        free(cols);
        map->obstacleCount = counter;
        return obstacles;
      }
    }
    free(cols);
  }
  map->obstacleCount = counter;
  return obstacles;
}


lane_t* initLanes(map_t *map, const settings_t settings) {
  lane_t *lanes = (lane_t*)malloc(sizeof(lane_t) * map->height);
  int *road_lanes = randomUniqueNumbers(1, map->height - 2, settings.roadNumber);
  map->roadCount = settings.roadNumber;
  lanes[0] = FINISH;
  for (int i = 1; i < map->height; i++) {
    lanes[i] = GRASS;
  }
  for (int i = 0; i < settings.roadNumber; i++) {
    lanes[road_lanes[i]] = (lane_t)((int)random() % 2);
  }
  free(road_lanes);
  return lanes;
}

//                                                                                      FIX THIS!!!!!!!!!!!!!!!!
car_t initCar(const map_t map, int yPosition) {
  if (yPosition == -1)
    yPosition = yPositionOfRoad(map, (int)random() % map.roadCount + 1);
  const carDirection_t b = (map.lanes[yPosition] == ROAD_LR) ? MOVES_RIGHT : MOVES_LEFT;
  const int x = (b == MOVES_RIGHT) ? 0 : map.width - 1;
  const car_t car = (car_t){
    .position = (position_t){yPosition, x},
    .good = 0,
    .length = 1,
    .regeneration = 0,
    .behaviour = (carBehaviour_t)((int)random() % 2),
    .direction = b,
    .dead = 0,
    .careful = (char)!(random() % 3),
    .delayBetweenMoves = (int)random()%10 + 5
  };
  return car;
}

car_t* initCars(const map_t map, const settings_t settings) {
  car_t *cars = (car_t*)malloc(sizeof(car_t) * settings.maxCarsNumber);
  int *starting_lanes = randomUniqueNumbers(1, settings.roadNumber, settings.maxCarsNumber);
  for (int i = 0; i < settings.maxCarsNumber; i++) {
    cars[i] = initCar(map, yPositionOfRoad(map, starting_lanes[i]));
  }
  free(starting_lanes);
  return cars;
}

map_t initMap(const settings_t settings){
  map_t map;
  map.width = settings.mapWidth;
  map.height = settings.mapHeight;
  map.position = MAP_POSITION;
  map.border.borderChar = '#';
  map.border.position = (position_t){map.position.y-1, map.position.x-1};
  map.roadCount = 0;
  map.lanes = initLanes(&map, settings);
  map.obstacles = initObstacles(&map, settings);
  return map;
}


player_t initPlayer(const settings_t settings) {
  player_t player;
  player.position.x = settings.mapWidth / 2;
  player.position.y = settings.mapHeight - 1;
  player.delayBetweenMoves = settings.playerDelay;
  player.regeneration = 0;
  return player;
}


settings_t initSettings() {
  FILE *file = fopen("CONFIG.txt", "r");
  settings_t settings;
  fscanf(file, "MAP_HEIGHT %d\n", &settings.mapHeight); // NOLINT(*-err34-c)
  fscanf(file, "MAP_WIDTH %d\n", &settings.mapWidth);// NOLINT(*-err34-c)
  fscanf(file, "PLAYER_DELAY %d\n" , &settings.playerDelay);// NOLINT(*-err34-c)
  fscanf(file, "CAR_MIN_DELAY %d\n" , &settings.carMinDelay);// NOLINT(*-err34-c)
  fscanf(file, "CAR_MAX_DELAY %d\n" , &settings.carMaxDelay);// NOLINT(*-err34-c)
  fscanf(file, "MIN_CARS_NUMBER %d\n", &settings.minCarsNumber);// NOLINT(*-err34-c)
  fscanf(file, "MAX_CARS_NUMBER %d\n", &settings.maxCarsNumber);// NOLINT(*-err34-c)
  fscanf(file, "ROAD_NUMBER %d\n", &settings.roadNumber);// NOLINT(*-err34-c)
  fclose(file);

  // settings.mapHeight = readSetting("MAP_HEIGHT", file);
  // settings.mapWidth = readSetting("MAP_WIDTH", file);
  // settings.playerDelay = readSetting("PLAYER_DELAY", file);
  // settings.carMinDelay = readSetting("CAR_MIN_DELAY", file);
  // settings.carMaxDelay = readSetting("CAR_MAX_DELAY", file);
  // settings.minCarsNumber = readSetting("MIN_CARS_NUMBER", file);
  // settings.maxCarsNumber = readSetting("MAX_CARS_NUMBER", file);

  return settings;
}


game_t initGame() {
  game_t game;
  game.settings = initSettings();
  game.map = initMap(game.settings);
  game.player = initPlayer(game.settings);
  game.cars = initCars(game.map, game.settings);
  game.gameState = PLAYING;
  game.updates_counter = 0;
  return game;
}

//************************************************
//*******************FUNCTIONS********************
//************************************************

//-----------------printing stuff-----------------
void printBorder(const map_t map){
  attron(COLOR_PAIR(COLOR_BORDER));
  mvaddch(map.border.position.y, map.border.position.x, '+');
  mvaddch(map.border.position.y + map.height + 1, map.border.position.x + map.width + 1, '+');
  mvaddch(map.border.position.y, map.border.position.x + map.width +1, '+');
  mvaddch(map.border.position.y + map.height + 1, map.border.position.x, '+');
  for(int x = 1; x < (map.width) + 1; x++){
    mvaddch(map.border.position.y, map.border.position.x + x, '-');
    mvaddch(map.border.position.y + map.height + 1, map.border.position.x + x, '-');
  }
  for(int y = 0; y < map.height; y++){
    mvaddch(map.border.position.y + y + 1, map.border.position.x, '|');
    mvaddch(map.border.position.y + y + 1, map.border.position.x + map.width + 1, '|');
  }
}


void printLane(const position_t position, const lane_t lane, const int width) {
  char ch = ' ';
  switch(lane) {
    case GRASS:
      attron(COLOR_PAIR(COLOR_GRASS));
      ch = '#';
      break;
    case ROAD_LR:
      attron(COLOR_PAIR(COLOR_ROAD));
      ch = '>';
      break;
    case ROAD_RL:
      attron(COLOR_PAIR(COLOR_ROAD));
      ch = '<';
      break;
    case FINISH:
      attron(COLOR_PAIR(COLOR_FINISH));
      ch = '=';
    default:;
  }
  for(int x = 0; x < width; x++)
    mvaddch(position.y, position.x + x, ch);
}

void printObstacles(const game_t game) {
  for (int i = 0; i < game.map.obstacleCount; i++) {
    attron(COLOR_PAIR(COLOR_OBSTACLE));
    mvaddch(game.map.obstacles[i].y + game.map.position.y, game.map.obstacles[i].x + game.map.position.x, '&');
  }
}


void printMap(const map_t map){
  printBorder(map);
  for(int y = 0; y < map.height; y++) {
    printLane((position_t){ map.position.y + y, map.position.x}, map.lanes[y], map.width);
  }
}


void printPlayer(const game_t game){
  attron(COLOR_PAIR(COLOR_FROG));
  mvaddch(game.player.position.y + game.map.position.y, game.player.position.x + game.map.position.x, 'O');
}


void printCars(const game_t game) {
  for(int i = 0; i < game.settings.maxCarsNumber; i++) {
    if (game.cars[i].dead)
      continue;
    if (game.cars[i].good)
      attron(COLOR_PAIR(COLOR_GOOD_CAR));
    else
      attron(COLOR_PAIR(COLOR_BAD_CAR));
    mvaddch(game.cars[i].position.y + game.map.position.y, game.cars[i].position.x + game.map.position.x, '%');
  }
}

void printStats(const game_t game) {
  attron(COLOR_PAIR(COLOR_TEXT));
  mvprintw(game.map.position.y + game.map.height + 1, game.map.position.x -1, "Time: %.2fs", (double)game.updates_counter / 100.0);
}


void printGame(const game_t game){
  clear();

  if (game.gameState == PLAYING) {
    printMap(game.map);
    printStats(game);
    printPlayer(game);
    printCars(game);
    printObstacles(game);
    return;
  }
  if (game.gameState == WIN) {
    attron(COLOR_PAIR(COLOR_TEXT));
    mvprintw(LINES/2, COLS/2 - 4, "YOU WIN");
    mvprintw(LINES/2 + 1, COLS/2 - 8, "Your time: %.2fs", (double)game.updates_counter / 100.0);
    return;
  }
  if (game.gameState == LOST) {
    attron(COLOR_PAIR(COLOR_TEXT));
    mvprintw(LINES/2, COLS/2, "YOU LOOSE");
  }
  if (game.gameState == STOPPED) {
    attron(COLOR_PAIR(COLOR_TEXT));
    mvprintw(LINES/2, COLS/2, "GAME ABORTED");
  }
}


//-----------------control stuff------------------
char checkCrash(const game_t game) {
  for(int i = 0; i < game.settings.maxCarsNumber; i++) {
    if (game.cars[i].dead)
      continue;
    if (game.cars[i].position.x == game.player.position.x && game.cars[i].position.y == game.player.position.y)
      return 1;
  }
  return 0;
}


char checkWin(const game_t game) {
  if (!game.player.position.y)
    return 1;
  return 0;
}


char checkStop(const car_t car, const position_t playerPosition) {
  if (
    car.careful &&
    (car.position.y - playerPosition.y < 2 && car.position.y - playerPosition.y > -2) &&
    (
      (car.direction == MOVES_LEFT && (car.position.x - playerPosition.x) > 0 && (car.position.x - playerPosition.x) < 4) ||
      (car.direction == MOVES_RIGHT && (playerPosition.x - car.position.x) > 0 && (playerPosition.x - car.position.x) < 4)
    )
  )
    return 1;



  return 0;
}


char carDisappear(car_t* car, const settings_t settings) {
  if (
      car->behaviour == DISAPPEARING &&
      ((car->direction == MOVES_RIGHT && car->position.x == settings.mapWidth-1) ||
      (car->direction == MOVES_LEFT && car->position.x == 0))
      ) {
    car->dead = 1;
    return 1;
  }
  return 0;
}


void movePlayer(game_t *game, const position_t move){
  if(game->player.regeneration != 0)
    return;
  if (
    game->player.position.x + move.x < 0 ||
    game->player.position.x + move.x >= game->map.width ||
    game->player.position.y + move.y < 0 ||
    game->player.position.y + move.y >= game->map.height
    )
    return;
  int x = game->player.position.x + move.x;
  int y = game->player.position.y + move.y;
  for (int i = 0; i < game->map.obstacleCount; i++)
    if (game->map.obstacles[i].x == x && game->map.obstacles[i].y == y)
      return;
  game->player.position.x = x;
  game->player.position.y = y;
  game->player.regeneration = game->player.delayBetweenMoves;
}


void moveCar(car_t* car, const map_t map, const settings_t settings) {
  if (car->direction == MOVES_RIGHT && car->position.x == map.width-1)
    car->position.x = 0;
  else
  if (car->direction == MOVES_LEFT && car->position.x == 0)
    car->position.x = map.width-1;
  else
    car->position.x += (car->direction == MOVES_RIGHT) ? 1 : -1;

    car->regeneration = car->delayBetweenMoves;
}


void moveCars(const game_t* game) {
  for(int i = 0; i < game->settings.maxCarsNumber; i++) {
    const car_t car = game->cars[i];
    if (car.regeneration != 0)
      continue;
    if (car.dead) {
      if (!((int)random()%1000))
        game->cars[i] = initCar(game->map, -1);
      continue;
    }
    if (carDisappear(&game->cars[i], game->settings))
      continue;
    if (checkStop(car, game->player.position))
      continue;

    moveCar(&game->cars[i], game->map, game->settings);
  }
}

void regeneratePlayer(player_t* player){
  if(player->regeneration != 0)
    player->regeneration -= 1;
}


void regenerateCars(const game_t *game) {
  for(int i = 0; i < game->settings.maxCarsNumber; i++) {
    if(game->cars[i].regeneration != 0)
      game->cars[i].regeneration -= 1;
  }
}


void input(const char key, game_t *game) {
  switch (key) {
    case 'w':
    case 'W':
      movePlayer(game, UP);
      break;
    case 's':
    case 'S':
      movePlayer(game, DOWN);
      break;
    case 'a':
    case 'A':
      movePlayer(game, LEFT);
      break;
    case 'd':
    case 'D':
      movePlayer(game, RIGHT);
      break;
    case 'q':
    case 'Q':
      game->gameState = STOPPED;
      break;
    default:;
  }
}

//this loop gets called repeatedly when the game is running - every update calls other functions to update current state of the game and print the game to the screen
void update(game_t *game) {
  moveCars(game);
  input(getch(), game);
  regeneratePlayer(&game->player);
  regenerateCars(game);
  if (checkCrash(*game))
    game->gameState = LOST;
  if (checkWin(*game))
    game->gameState = WIN;
  printGame(*game);
  game->updates_counter++;
}


void freeGame(const game_t* game) {
  free(game->map.lanes);
  free(game->cars);
  free(game->map.obstacles);
}


void colors() {
  start_color();
  init_color(10, 0, 0, 0);
  init_pair(COLOR_FROG, COLOR_BLACK, COLOR_GREEN);
  init_pair(COLOR_BAD_CAR, COLOR_WHITE, COLOR_RED);
  init_pair(COLOR_GOOD_CAR, COLOR_WHITE, COLOR_BLUE);
  init_pair(COLOR_GRASS, COLOR_GREEN, COLOR_YELLOW);
  init_pair(COLOR_ROAD, 10, 10);
  init_pair(COLOR_BORDER, COLOR_WHITE, COLOR_WHITE);
  init_pair(COLOR_FINISH, COLOR_MAGENTA, COLOR_MAGENTA);
  init_pair(COLOR_TEXT, COLOR_WHITE, COLOR_BLACK);
  init_pair(COLOR_OBSTACLE, COLOR_YELLOW, COLOR_GREEN);
}

//basic setup of ncurses screen;
void setup() {
  initscr();
  clear();
  noecho();
  nodelay(stdscr, 1);
  curs_set(0);
  srandom(time(NULL));
  colors();
}





int main() {
  setup();
  const struct timespec ts = {0, 10000000};

  game_t game = initGame();
  while(game.gameState == PLAYING) {
    update(&game);
    mvprintw(0, 0, "%d", game.map.obstacleCount);
    nanosleep(&ts, NULL);
  }
  while (getch() == ERR){}
  freeGame(&game);
  clear();
  endwin();
  return 0;
}