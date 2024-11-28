#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>

//************************************************
//*******************DEFINES**********************
//************************************************

#define NUMBER_OF_CARS 10
#define PLAYER_DELAY 30

#define UP (position_t){ -1, 0 }
#define DOWN (position_t){ 1, 0 }
#define LEFT (position_t){ 0, -1 }
#define RIGHT (position_t){ 0, 1 }

#define MAP_WIDTH 11
#define MAP_HEIGHT 20
#define MAP_POSITION ((position_t){ 2, COLS/2 - 31/2 })

//************************************************
//*********************ENUMS**********************
//************************************************

typedef enum {
  ROAD_LR,
  ROAD_RL,
  GRASS
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
  carBehaviour_t behaviour;
  carDirection_t direction;
}car_t;


typedef struct {

}stork_t;


typedef struct {
  map_t map;
  player_t player;
  car_t* cars;
  gameState_t gameState;
  unsigned long int updates_counter;
}game_t;



//************************************************
//*****************BASIC-FUNCTIONS****************
//************************************************


int* randomUniqueNumbers(const int min, int max, const int number) {
  int highest_possible_number = max;
  int lowest_possible_number = min;
  max = highest_possible_number - lowest_possible_number + 1;
  int *numbers = malloc(sizeof(int) * number);
  for (int iteration = 0, chosen = 0; iteration < max && chosen < number; iteration++) {
    const int remaining_numbers = max - iteration;
    const int remaining_to_choose = number - chosen;
    if ((int)random() % remaining_numbers < remaining_to_choose)
      numbers[chosen++] = iteration + min;
  }
  return numbers;
}


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


lane_t* initLanes(map_t *map) {
  lane_t *lanes = (lane_t*)malloc(sizeof(lane_t) * map->height);
  int *road_lanes = randomUniqueNumbers(1, map->height - 2, NUMBER_OF_CARS);
  map->roadCount = NUMBER_OF_CARS;
  for (int i = 0; i < map->height; i++) {
    lanes[i] = GRASS;
  }
  for (int i = 0; i < NUMBER_OF_CARS; i++) {
    lanes[road_lanes[i]] = (lane_t)((int)random() % 2);
  }
  free(road_lanes);
  return lanes;
}


car_t initCar(const map_t map) {
  const int y = yPositionOfRoad(map, (int)random() % map.roadCount + 1);
  const carDirection_t b = (map.lanes[y] == ROAD_LR) ? MOVES_RIGHT : MOVES_LEFT;
  const int x = (b == MOVES_RIGHT) ? 0 : map.width - 1;
  const car_t car = (car_t){
    .position = (position_t){y, x},
    .good = 0,
    .length = 1,
    .regeneration = 0,
    .behaviour = (carBehaviour_t)((int)random() % 2),
    .direction = b,
    .dead = 0,
    .delayBetweenMoves = (int)random()%10 + 5
  };
  return car;
}

car_t* initCars(const map_t map, const int numCars) {
  car_t *cars = (car_t*)malloc(sizeof(car_t) * NUMBER_OF_CARS);
  int *starting_lanes = randomUniqueNumbers(1, NUMBER_OF_CARS, NUMBER_OF_CARS);
  for (int i = 0; i < NUMBER_OF_CARS; i++) {
    const int y = yPositionOfRoad(map, starting_lanes[i]);
    const int x = (int)random() % (map.width - 2) + 1;
    const carDirection_t b = (map.lanes[y] == ROAD_LR) ? MOVES_RIGHT : MOVES_LEFT;
    cars[i] = (car_t){
      .position = (position_t){y, x},
      .good = 0,
      .length = 1,
      .regeneration = 0,
      .behaviour = (carBehaviour_t)((int)random() % 2),
      .direction = b,
      .dead = 0,
      .delayBetweenMoves = (int)random()%10 + 5
    };
  }
  free(starting_lanes);
  return cars;
}

map_t initMap(const int mapWidth, const int mapHeight, const position_t position){
  map_t map;
  map.width = mapWidth;
  map.height = mapHeight;
  map.position = position;
  map.border.borderChar = '#';
  map.border.position = (position_t){position.y-1, position.x-1};
  map.roadCount = 0;
  map.lanes = initLanes(&map);
  return map;
}


player_t initPlayer(const position_t position, const int delay) {
  return (player_t){ .position = position, .delayBetweenMoves = delay, .regeneration = 0 };
}


game_t initGame() {
  game_t game;
  game.map = initMap(MAP_WIDTH, MAP_HEIGHT, MAP_POSITION);
  game.player = initPlayer( (position_t){MAP_HEIGHT-1, MAP_WIDTH/2}, PLAYER_DELAY);
  game.cars = initCars(game.map, 5);
  game.gameState = PLAYING;
  game.updates_counter = 0;
  return game;
}

//************************************************
//*******************FUNCTIONS********************
//************************************************

//-----------------printing stuff-----------------
void printBorder(const map_t map){
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
      ch = '.';
      break;
    case ROAD_LR:
      ch = '>';
      break;
    case ROAD_RL:
      ch = '<';
      break;
    default:;
  }
  for(int x = 0; x < width; x++)
    mvaddch(position.y, position.x + x, ch);
}


void printMap(const map_t map){
  attron(COLOR_PAIR(1));
  printBorder(map);
  attron(COLOR_PAIR(2));
  for(int x = 0; x < map.width; x++)
    mvaddch(map.position.y, map.position.x + x, '=');
  for(int y = 1; y < map.height; y++) {
    printLane((position_t){ map.position.y + y, map.position.x}, map.lanes[y], map.width);
  }
}


void printPlayer(const game_t game){
  mvaddch(game.player.position.y + game.map.position.y, game.player.position.x + game.map.position.x, 'O');
}


void printCars(const game_t game) {
  for(int i = 0; i < NUMBER_OF_CARS; i++) {
    if (!game.cars[i].dead)
      mvaddch(game.cars[i].position.y + game.map.position.y, game.cars[i].position.x + game.map.position.x, '%');
  }
}

void printStats(const game_t game) {
  mvprintw(game.map.position.y + game.map.height + 1, game.map.position.x -1, "Time: %.2fs", (double)game.updates_counter / 100.0);
}


void printGame(const game_t game){
  clear();

  if (game.gameState == PLAYING) {
    printMap(game.map);
    printStats(game);
    printPlayer(game);
    printCars(game);
    return;
  }
  if (game.gameState == WIN) {
    mvprintw(LINES/2, COLS/2 - 4, "YOU WIN");
    mvprintw(LINES/2 + 1, COLS/2 - 8, "Your time: %.2fs", (double)game.updates_counter / 100.0);
    return;
  }
  if (game.gameState == LOST) {
    mvprintw(LINES/2, COLS/2, "YOU LOOSE");
  }
  if (game.gameState == STOPPED) {
    mvprintw(LINES/2, COLS/2, "GAME ABORTED");
  }
}


//-----------------control stuff------------------
int checkCrash(const game_t game) {
  for(int i = 0; i < NUMBER_OF_CARS; i++) {
    if (game.cars[i].dead)
      continue;
    if (game.cars[i].position.x == game.player.position.x && game.cars[i].position.y == game.player.position.y)
      return 1;
  }
  return 0;
}


int checkWin(const game_t game) {
  if (!game.player.position.y)
    return 1;
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
  game->player.position.x += move.x;
  game->player.position.y += move.y;
  game->player.regeneration = game->player.delayBetweenMoves;
}


void moveCars(const game_t *game) {
  for(int i = 0; i < NUMBER_OF_CARS; i++) {
    if (game->cars[i].regeneration != 0)
      continue;
    if (game->cars[i].dead) {
      if (!((int)random()%1000))
        game->cars[i] = initCar(game->map);
      continue;
    }
    if (
      game->cars[i].behaviour == DISAPPEARING &&
      ((game->cars[i].direction == MOVES_RIGHT && game->cars[i].position.x == game->map.width-1) ||
      (game->cars[i].direction == MOVES_LEFT && game->cars[i].position.x == 0))
      )
      game->cars[i].dead = 1;
    else
    if (game->cars[i].direction == MOVES_RIGHT && game->cars[i].position.x == game->map.width-1)
      game->cars[i].position.x = 0;
    else
    if (game->cars[i].direction == MOVES_LEFT && game->cars[i].position.x == 0)
      game->cars[i].position.x = game->map.width-1;
    else
      game->cars[i].position.x += (game->cars[i].direction == MOVES_RIGHT) ? 1 : -1;

    game->cars[i].regeneration = game->cars[i].delayBetweenMoves; // NOLINT(*-misleading-indentation)
  }
}


void regeneratePlayer(player_t* player){
  if(player->regeneration != 0)
    player->regeneration -= 1;
}


void regenerateCars(car_t* cars) {
  for(int i = 0; i < NUMBER_OF_CARS; i++) {
    if(cars[i].regeneration != 0)
      cars[i].regeneration -= 1;
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


void update(game_t *game) {
  input(getch(), game);
  moveCars(game);
  regeneratePlayer(&game->player);
  regenerateCars(game->cars);
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
}




int main() {
  //basic setup: preparing the ncurses screen, defining timespec for delay between game updates
  initscr();
  clear();
  noecho();
  nodelay(stdscr, 1);
  curs_set(0);
  srandom(time(NULL));
  const struct timespec ts = {0, 10000000};
  start_color();
  init_color(10,0,0,0);
  init_pair(1, COLOR_GREEN, COLOR_GREEN);
  init_pair(2, COLOR_WHITE, 10);


  game_t game = initGame();
  while(game.gameState == PLAYING) {
    update(&game);
    nanosleep(&ts, NULL);
  }
  while (getch() == ERR) {

  }
  freeGame(&game);
  clear();
  endwin();
  return 0;
}