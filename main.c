#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>
#include <string.h>
#include <math.h>

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
#define COLOR_STORK 19

#define BORDER_CHAR '#'
#define STORK_CHAR '*'

#define UP (position_t){ -1, 0 }
#define DOWN (position_t){ 1, 0 }
#define LEFT (position_t){ 0, -1 }
#define RIGHT (position_t){ 0, 1 }

#define MAP_POSITION ((position_t){ 5, COLS/2 - map.width/2 })

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
  int width;
  int height;
  lane_t *lanes;
  int roadCount;
  position_t *obstacles;
  int obstacleCount;
  int carCount;
}map_t;


typedef struct {
  position_t position;
  int delayBetweenMoves;
  int regeneration;
  int inCar;
}player_t;


typedef struct {
  position_t position;
  int length;
  int delayBetweenMoves;
  int regeneration;
  char good;
  char dead;
  char careful;
  char new;
  carBehaviour_t behaviour;
  carDirection_t direction;
}car_t;


typedef struct {
  position_t position;
  int delayBetweenMoves;
  int regeneration;
  char dead;
}stork_t;


typedef struct {
  int mapWidth;
  int mapHeight;
  int playerDelay;
  int carMinDelay;
  int carMaxDelay;
  int minCarsNumber;
  int maxCarsNumber;
  int carMinLength;
  int carMaxLength;
  int roadNumber;
  int maxObstacles;
  int minObstacles;
}settings_t;


typedef struct {
  map_t map;
  player_t player;
  car_t* cars;
  stork_t stork;
  gameState_t gameState;
  unsigned long int updatesCounter;
  unsigned long int movesCounter;
  settings_t settings;
  unsigned short int level;
  unsigned int score;
}game_t;


typedef struct {
  unsigned int score;
  char name[16];
}score_t;


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


//returns position of road number n on map
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


unsigned int calculateScore(const game_t game) {
  int score = 200, timeDeduction = 0, moveDeduction = 0;
  timeDeduction = (int)(game.updatesCounter/100);
  if (timeDeduction > 100)
    timeDeduction = 100;
  moveDeduction = (int)(game.movesCounter/2);
  if (moveDeduction > 100)
    moveDeduction = 100;
  score -= timeDeduction + moveDeduction;
  return score;
}

//returns the highest length of a car that fits on a specific lane
//(the highest value returned is car's max length from config even it there would be space for longer car)
int highestFittingLength(const car_t* cars, const int y, const settings_t settings) {
  int length = settings.mapWidth - 4;
  for (int i = 0; i < settings.maxCarsNumber; i++)
    if (!cars[i].dead && cars[i].position.y == y)
      length -= cars[i].length + 1;
  if (length > settings.carMaxLength)
    return settings.carMaxLength;
  return length;
}


//************************************************
//**********************INITS*********************
//************************************************


position_t* initObstacles(map_t* map, const settings_t settings) {
  const int maxMapObstacles = (settings.mapHeight - 2 - settings.roadNumber) * settings.maxObstacles;
  position_t *obstacles = malloc(sizeof(position_t) * maxMapObstacles);
  int counter = 0;
  for (int i = 1; i < map->height-1; i++) {
    if (map->lanes[i] != GRASS)
      continue;

    int j =  (int)random()%(settings.maxObstacles - settings.minObstacles + 1) + settings.minObstacles;
    int *cols = randomUniqueNumbers(0, map->width - 1, j);
    for (; j>0; j--) {
      position_t obstacle;
      obstacle.y = i;
      obstacle.x = cols[j-1];
      obstacles[counter++] = obstacle;
      if (counter == maxMapObstacles) {
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


car_t initCar(const map_t map, const int yPosition, const int length,const settings_t settings) {
  const carDirection_t b = (map.lanes[yPosition] == ROAD_LR) ? MOVES_RIGHT : MOVES_LEFT;
  const int x = (b == MOVES_RIGHT) ? 0 : settings.mapWidth - 1;
  const car_t car = (car_t){
    .position = (position_t){yPosition, x},
    .good = (char)!(random()%6),
    .length = length,
    .regeneration = 0,
    .behaviour = (carBehaviour_t)(int)random() % 2,
    .direction = b,
    .dead = 0,
    .new = 1,
    .careful = (char)!(random() % 6),
    .delayBetweenMoves = (int)random()%(settings.carMaxDelay - settings.carMinDelay + 1) + settings.carMinDelay,
  };
  return car;
}


car_t* initCars(const map_t map, const settings_t settings) {
  car_t *cars = (car_t*)malloc(sizeof(car_t) * settings.maxCarsNumber);
  int *starting_lanes = randomUniqueNumbers(1, settings.roadNumber, settings.minCarsNumber);
  for (int i = 0; i < settings.minCarsNumber; i++) {
    const int length = (int)random() % (settings.carMaxLength - settings.carMinLength + 1) + settings.carMinLength;
    cars[i] = initCar(map, yPositionOfRoad(map, starting_lanes[i]), length,settings);
  }
  for (int i = settings.minCarsNumber; i < settings.maxCarsNumber; i++) {
    cars[i] = initCar(map, -1, 1,settings);
    cars[i].dead = 1;
  }
  free(starting_lanes);
  return cars;
}


map_t initMap(const settings_t settings){
  map_t map;
  map.width = settings.mapWidth;
  map.height = settings.mapHeight;
  map.position = MAP_POSITION;
  map.roadCount = 0;
  map.lanes = initLanes(&map, settings);
  map.obstacles = initObstacles(&map, settings);
  map.carCount = settings.minCarsNumber;
  return map;
}


player_t initPlayer(const settings_t settings) {
  player_t player;
  player.position.x = settings.mapWidth / 2;
  player.position.y = settings.mapHeight - 1;
  player.delayBetweenMoves = settings.playerDelay;
  player.regeneration = 0;
  player.inCar = -1;
  return player;
}


stork_t initStork(const settings_t settings, const int level) {
  stork_t stork;
  stork.position.x = settings.mapWidth / 2;
  stork.position.y = 0;
  stork.delayBetweenMoves = settings.playerDelay * 4;
  stork.regeneration = 0;
  stork.dead = (level == 3)?0:1;
  return stork;
}


void configError(const int i) {
  clear();
  nodelay(stdscr, 0);
  switch (i) {
    case 0:
      mvprintw(0,0,"Map height must be greater than 2\n");
    break;
    case 1:
      mvprintw(0,0,"Map width must be greater than 5\n");
    break;
    case 2:
      mvprintw(0,0,"Incorrect car delays\n");
    break;
    case 3:
      mvprintw(0,0,"Min cars number must be <= road number\n");
    case 4:
      mvprintw(0,0,"Incorrect obstacle numbers\n");
    break;
    case 5:
      mvprintw(0,0,"Road number must be less or equal to map height - 2\n");
    break;
    case 6:
      mvprintw(0,0,"No CONFIG.txt file\n");
    break;
    default:
      mvprintw(0,0,"CONFIG ERROR\n");
  }
  refresh();
  getch();
  clear();
  endwin();
  exit(1);
}


void verifyConfig(const settings_t s) {
  if (s.mapHeight < 3)
    configError(0);
  if (s.mapWidth < 6)
    configError(1);
  if (s.carMaxDelay < s.carMinDelay)
    configError(2);
  if (s.minCarsNumber > s.roadNumber)
    configError(3);
  if (s.minCarsNumber > s.maxCarsNumber)
    configError(4);
  if (s.roadNumber > s.mapHeight - 2)
    configError(5);
}


settings_t readSettings() {
  FILE *file = fopen("CONFIG.txt", "r");
  if (file == NULL) {
    configError(6);
  }
  settings_t settings;
  fscanf(file, "MAP_HEIGHT %d\n", &settings.mapHeight); // NOLINT(*-err34-c)
  fscanf(file, "MAP_WIDTH %d\n", &settings.mapWidth);// NOLINT(*-err34-c)
  fscanf(file, "PLAYER_DELAY %d\n" , &settings.playerDelay);// NOLINT(*-err34-c)
  fscanf(file, "LV1_CAR_MIN_DELAY %d\n" , &settings.carMinDelay);// NOLINT(*-err34-c)
  fscanf(file, "LV1_CAR_MAX_DELAY %d\n" , &settings.carMaxDelay);// NOLINT(*-err34-c)
  fscanf(file, "MIN_CARS_NUMBER %d\n", &settings.minCarsNumber);// NOLINT(*-err34-c)
  fscanf(file, "LV1_MAX_CARS_NUMBER %d\n", &settings.maxCarsNumber);// NOLINT(*-err34-c)
  fscanf(file, "CAR_MIN_LENGTH %d\n" , &settings.carMinLength);// NOLINT(*-err34-c)
  fscanf(file, "CAR_MAX_LENGTH %d\n" , &settings.carMaxLength);// NOLINT(*-err34-c)
  fscanf(file, "LV1_ROAD_NUMBER %d\n", &settings.roadNumber);// NOLINT(*-err34-c)
  fscanf(file, "LV2_MAX_OBSTACLES_PER_LANE %d\n", &settings.maxObstacles);// NOLINT(*-err34-c)
  fscanf(file, "LV2_MIN_OBSTACLES_PER_LANE %d\n", &settings.minObstacles);// NOLINT(*-err34-c)
  fclose(file);
  verifyConfig(settings);
  return settings;
}


settings_t initSettings(const int level) {
  settings_t settings = readSettings();
  settings.carMinDelay -= (level - 1) * settings.carMinDelay / 4;
  settings.carMaxDelay /= level;
  settings.maxCarsNumber += (level - 1) * 4;
  settings.roadNumber += (settings.mapHeight - 2 - settings.roadNumber) / 4 * (level - 1);
  settings.minObstacles = (level - 1) * settings.minObstacles;
  settings.maxObstacles = (level - 1) * settings.maxObstacles;
  if (settings.maxObstacles > settings.mapWidth / 3)
    settings.maxObstacles = settings.mapWidth / 3;
  return settings;
}


game_t initGame(const unsigned short int level) {
  game_t game;
  game.level = level;
  game.settings = initSettings(level);
  game.map = initMap(game.settings);
  game.player = initPlayer(game.settings);
  game.cars = initCars(game.map, game.settings);
  game.gameState = PLAYING;
  game.updatesCounter = 0;
  game.movesCounter = 0;
  game.stork = initStork(game.settings, level);
  game.score = 0;
  return game;
}

//************************************************
//*******************FUNCTIONS********************
//************************************************

//-----------------printing stuff-----------------
void printBorder(const map_t map){
  attron(COLOR_PAIR(COLOR_BORDER));
  const int y = map.position.y - 4, x = map.position.x - 1;
  const int rows[4] = {y, y + 3, map.height + y + 4, map.height + y + 7};
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < map.width + 2; j++) {
      mvaddch(rows[i], x + j, BORDER_CHAR);
    }
  }
  const int cols[2] = {x, x + map.width + 1};
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < map.height + 7; j++) {
      mvaddch(y + j, cols[i], BORDER_CHAR);
    }
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

void printObstacles(const map_t map) {
  for (int i = 0; i < map.obstacleCount; i++) {
    attron(COLOR_PAIR(COLOR_OBSTACLE));
    mvaddch(map.obstacles[i].y + map.position.y, map.obstacles[i].x + map.position.x, '&');
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


void printCar(const car_t car, const map_t map) {
  if (!car.new) {
    for (int i = 0; i < car.length; i++) {
      mvaddch(
        car.position.y + map.position.y,
        (car.position.x + ((car.direction==MOVES_RIGHT)?(-i):(i)) + map.width)%map.width + map.position.x ,
        '%');
    }
    return;
  }
  for (int i = 0; i < car.length; i++) {
    if (
      car.direction == MOVES_LEFT &&
      car.position.x + i >= map.width)
      break;
    if (
      car.direction == MOVES_RIGHT &&
      car.position.x - i < 0)
      break;
    mvaddch(car.position.y + map.position.y, car.position.x + map.position.x + ((car.direction==MOVES_RIGHT)?(-i):(i)), '%');
  }

}


void printCars(const game_t game) {
  for(int i = 0; i < game.settings.maxCarsNumber; i++) {
    if (game.cars[i].dead)
      continue;
    if (game.cars[i].good)
      attron(COLOR_PAIR(COLOR_GOOD_CAR));
    else
      attron(COLOR_PAIR(COLOR_BAD_CAR));
    printCar(game.cars[i], game.map);
  }
}

void printStats(const game_t game) {
  attron(COLOR_PAIR(COLOR_TEXT));
  mvprintw(game.map.position.y - 3, game.map.position.x, "Filip Berka");
  mvprintw(game.map.position.y - 2, game.map.position.x, "203163");
  mvprintw(game.map.position.y + game.map.height + 1, game.map.position.x, "Time: %.2fs", (double)game.updatesCounter / 100.0);
  mvprintw(game.map.position.y + game.map.height + 2, game.map.position.x, "Moves: %lu", game.movesCounter);
}


void printStork(const game_t game) {
  if (game.stork.dead)
    return;
  attron(COLOR_PAIR(COLOR_STORK));
  mvaddch(game.map.position.y + game.stork.position.y, game.map.position.x + game.stork.position.x, STORK_CHAR);
}


void printGame(const game_t game){
  clear();
  if (game.gameState == PLAYING) {
    printMap(game.map);
    printStats(game);
    printCars(game);
    printPlayer(game);
    printObstacles(game.map);
    printStork(game);
    return;
  }
  const struct timespec t = {1,0};
  attron(COLOR_PAIR(COLOR_TEXT));
  if (game.gameState == WIN) {
    mvprintw(LINES/2 - 1, COLS/2 - 8, "LEVEL %d COMPLETED", game.level);
    mvprintw(LINES/2, COLS/2 - 8, "Your time: %.2fs", (double)game.updatesCounter / 100.0);
    mvprintw(LINES/2 + 1, COLS/2 - 8, "Your score: %d", calculateScore(game));
  }
  else if (game.gameState == LOST) {
    mvprintw(LINES/2 , COLS/2 - 3, "YOU LOOSE");
  }
  else {
    mvprintw(LINES/2, COLS/2 - 5, "GAME ABORTED");
  }
  refresh();
  nanosleep(&t, NULL);
  while (getch() == ERR){}
}


void printMenu() {
  attron(COLOR_PAIR(COLOR_TEXT));
  clear();
  mvprintw(0, 0, "FROG GAME BY FILIP BERKA");
  mvprintw(1, 0, "1. Start game");
  mvprintw(2, 0, "2. Show recorded game");
  mvprintw(3, 0, "3. Show leaderboard");
  mvprintw(4, 0, "4. Exit");

}

int scoreCompare(const void* score1, const void* score2) {
  return (int)((score_t*)score2)->score - (int)((score_t*)score1)->score;
}

void printLeaderboard() {
  FILE *f = fopen("SCORES.txt", "r");
  if (f == NULL) {
    printf("No scores saved");
    return;
  }
  char count = 0;
  score_t currentScore;
  score_t topTen[11];
  while (fscanf(f, "%s %u", currentScore.name, &currentScore.score) != EOF) { // NOLINT(*-err34-c)
    if (count < 10) {
      topTen[count++] = currentScore;
      continue;
    }
    topTen[10] = currentScore;
    qsort(topTen, 11, sizeof(score_t), scoreCompare);
  }

  if (count > 10)
    count = 10;
  clear();
  for (int i = 0; i < count; i++) {
    mvprintw(i + 2, COLS/2 - 15, " | %15s | %3u |", topTen[i].name, topTen[i].score);
  }
  refresh();
  getch();
}

//-----------------control stuff------------------
char checkCarCollision(const car_t car, const map_t map, const position_t playerPosition) {
  if (car.position.y != playerPosition.y)
    return 0;
  if (!car.new) {
    for (int i = 0; i < car.length; i++) {
      if ((car.position.x + ((car.direction==MOVES_RIGHT)?(-i):(i)) + map.width)%map.width == playerPosition.x)
      return 1;
    }
    return 0;
  }
  for (int i = 0; i < car.length; i++) {
    if (
      car.direction == MOVES_LEFT &&
      car.position.x + i >= map.width)
      return 0;
    if (
      car.direction == MOVES_RIGHT &&
      car.position.x - i < 0)
      return 0;
    if (car.position.x + ((car.direction==MOVES_RIGHT)?(-i):(i)) == playerPosition.x)
      return 1;
  }
  return 0;
}


char checkCrash(const game_t* game) {
  if (game->player.position.x == game->stork.position.x && game->player.position.y == game->stork.position.y && !game->stork.dead)
    return 1;
  for(int i = 0; i < game->settings.maxCarsNumber; i++) {
    if (game->cars[i].dead)
      continue;
    if (!checkCarCollision(game->cars[i], game->map, game->player.position))
      continue;
    if (game->cars[i].good && game->player.inCar == i) {
      return 0;
    }
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


char checkCarAhead(const car_t car, const car_t* cars, const settings_t settings) {
  for(int i = 0; i < settings.maxCarsNumber; i++) {
    if (cars[i].dead)
      continue;
    if (car.position.y != cars[i].position.y)
      continue;
    if (car.position.x == cars[i].position.x)
      continue;
    if (
      car.direction == MOVES_LEFT &&
      (car.position.x - cars[i].position.x - cars[i].length + settings.mapWidth + 1) % settings.mapWidth < 3)
      return 1;
    if (
      car.direction == MOVES_RIGHT &&
      (cars[i].position.x - car.position.x - cars[i].length + settings.mapWidth + 1) % settings.mapWidth < 3)
      return 1;
  }
  return 0;
}


char carDisappear(map_t* map, car_t* car, const settings_t settings) {
  if (
      car->behaviour == DISAPPEARING &&
      ((car->direction == MOVES_RIGHT && car->position.x == settings.mapWidth-1) ||
      (car->direction == MOVES_LEFT && car->position.x == 0))) {
    if (map->carCount == settings.minCarsNumber) {
      car->behaviour = WRAPPING;
      car->new = 0;
      return 0;
    }
    if (car->length == 1) {
      map->carCount--;
      car->dead = 1;
      car->position.x = -1;
      car->position.y = -1;
    }else {
      car->length--;
      car->regeneration = car->delayBetweenMoves;
    }
    return 1;
  }
  return 0;
}


void movePlayer(game_t *game, const position_t move, const char movedByCar){
  const int x = (game->player.position.x + move.x + game->settings.mapWidth) % game->settings.mapWidth;
  const int y = (game->player.position.y + move.y + game->settings.mapHeight) % game->settings.mapHeight;
  for (int i = 0; i < game->map.obstacleCount; i++)
    if (game->map.obstacles[i].x == x && game->map.obstacles[i].y == y)
      return;
  game->player.position.x = x;
  game->player.position.y = y;
  if (!movedByCar) {
    game->player.regeneration = game->player.delayBetweenMoves;
    game->movesCounter++;
  }

}


void moveCar(car_t* car, const map_t map, const settings_t settings) {
  if (car->direction == MOVES_RIGHT && car->position.x == map.width-1) {
    if ((int)random() % 10) {
      car->position.x = 0;
      car->new = 0;
    }else
      car->behaviour = DISAPPEARING;
  }
  else
  if (car->direction == MOVES_LEFT && car->position.x == 0) {
    if ((int)random() % 10) {
      car->position.x = map.width-1;
      car->new = 0;
    }else
      car->behaviour = DISAPPEARING;

  }
  else
    car->position.x += (car->direction == MOVES_RIGHT) ? 1 : -1;

    car->regeneration = car->delayBetweenMoves;
}


void moveStork(stork_t* stork, const position_t playerPosition) {
  if (stork->regeneration || stork->dead)
    return;
  if (playerPosition.x > stork->position.x)
    stork->position.x++;
  else if (playerPosition.x < stork->position.x)
    stork->position.x--;
  if (playerPosition.y > stork->position.y)
    stork->position.y++;
  else if (playerPosition.y < stork->position.y)
    stork->position.y--;
  stork->regeneration = stork->delayBetweenMoves;
}


void createCar(game_t* game, const int carIndex) {
  const int yPosition = yPositionOfRoad(game->map, (int)random() % game->settings.roadNumber + 1);
  const int length = highestFittingLength(game->cars, yPosition, game->settings);
  if (length < game->settings.carMinLength)
    return;
  for (int i = 0; i < game->settings.maxCarsNumber; i++) {
    if (game->cars[i].dead)
      continue;
    if (game->cars[i].direction == MOVES_RIGHT && game->cars[i].position.x < (game->cars[i].length + 1)&& game->cars[i].position.y == yPosition)
      return;
    if (game->cars[i].direction == MOVES_LEFT && game->cars[i].position.x > game->map.width - (game->cars[i].length + 2) && game->cars[i].position.y == yPosition)
      return;
  }
  game->cars[carIndex] = initCar(game->map, yPosition,length, game->settings);
  game->map.carCount++;
}


void changeCarSpeed(car_t* car, const settings_t settings) {
  car->delayBetweenMoves += ((int)random()&2)?5:-5;
  if (car->delayBetweenMoves < settings.carMinDelay)
    car->delayBetweenMoves = settings.carMinDelay;
  if (car->delayBetweenMoves > settings.carMaxDelay)
    car->delayBetweenMoves = settings.carMaxDelay;
}


void moveCars(game_t* game) {
  for(int i = 0; i < game->settings.maxCarsNumber; i++) {
    const car_t car = game->cars[i];
    if (car.dead)
      continue;
    if (car.regeneration != 0)
      continue;
    if (carDisappear(&game->map, &game->cars[i], game->settings))
      continue;
    if (checkStop(car, game->player.position))
      continue;
    if (checkCarAhead(car, game->cars, game->settings))
      continue;
    if (!((int)random()%5))
      changeCarSpeed(&game->cars[i], game->settings);
    moveCar(&game->cars[i], game->map, game->settings);
    if (i == game->player.inCar)
      movePlayer(game, (car.direction == MOVES_LEFT)?(LEFT):(RIGHT), 1);
  }
}

void spawnNewCars(game_t* game) {
  for (int i = 0; i < game->settings.maxCarsNumber; i++)
    if (game->cars[i].dead)
      if (!((int)random()%1000))
        createCar(game, i);
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

void regenerateStork(stork_t* stork) {
  if(stork->regeneration != 0)
    stork->regeneration -= 1;
}

void enteredCar(game_t* game) {
  for (int i = 0; i < game->settings.maxCarsNumber; i++) {
    if (game->cars[i].good)
      if (checkCarCollision(game->cars[i], game->map, game->player.position)) {
        game->player.inCar = i;
        return;
      }
  }
  game->player.inCar = -1;
}


void input(const char key, game_t *game) {
  switch (key) {
    case 'w':
    case 'W':
      if (game->player.regeneration == 0 && game->player.position.y != 0) {
        movePlayer(game, UP, 0);
        enteredCar(game);
      }
      break;
    case 's':
    case 'S':
      if (game->player.regeneration == 0 && game->player.position.y != game->settings.mapHeight - 1) {
        movePlayer(game, DOWN, 0);
        enteredCar(game);
      }
      break;
    case 'a':
    case 'A':
      if (game->player.regeneration == 0 && game->player.position.x != 0 && game->player.inCar == -1)
        movePlayer(game, LEFT, 0);
      break;
    case 'd':
    case 'D':
      if (game->player.regeneration == 0 && game->player.position.x != game->settings.mapWidth - 1 && game->player.inCar == -1)
        movePlayer(game, RIGHT, 0);
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
  moveStork(&game->stork, game->player.position);
  spawnNewCars(game);
  if (checkCrash(game))
    game->gameState = LOST;
  input(getch(), game);
  regeneratePlayer(&game->player);
  regenerateCars(game);
  regenerateStork(&game->stork);
  if (checkWin(*game))
    game->gameState = WIN;
  printGame(*game);
  game->updatesCounter++;
}


void saveScore(score_t s) {
  FILE *f = fopen("SCORES.txt", "a+");
  fprintf(f, "%s %u\n", s.name, s.score);
  fclose(f);
}


void gameFinished(const unsigned int score) {
  clear();
  mvprintw(LINES / 2 - 3, COLS / 2 - 10, "YOU WIN!!!");
  mvprintw(LINES / 2 - 2, COLS / 2 - 10, "Your score: %d", score);
  mvprintw(LINES / 2 - 1, COLS / 2 - 10, "Input your name");
  mvprintw(LINES / 2 , COLS/2 - 10, "(max 15 characters):");
  move(LINES / 2 + 1, COLS / 2 - 10);
  nodelay(stdscr, 0);
  curs_set(1);
  echo();
  score_t s;
  s.score = score;
  scanw("%s", s.name);
  nodelay(stdscr, 1);
  curs_set(0);
  noecho();
  saveScore(s);
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
  init_pair(COLOR_STORK, COLOR_BLACK, COLOR_WHITE);
}


void setup() {
  initscr();
  clear();
  noecho();
  curs_set(0);
  srandom(time(NULL));
  colors();
}


void startGame() {
  nodelay(stdscr, 1);
  const struct timespec ts = {0, 10000000};
  unsigned int level = 1, totalScore = 0;
  game_t game;
  do {
    game = initGame(level++);
    while (game.gameState == PLAYING) {
      update(&game);
      nanosleep(&ts, NULL);
    }
    if (game.gameState != WIN)
      break;
    totalScore += calculateScore(game);
  }while (level < 4);
  if (game.gameState == WIN)
    gameFinished(totalScore);
  freeGame(&game);
  nodelay(stdscr, 0);
}


int main() {
  setup();
  while (1) {
    printMenu();
    const char ch = getch();
    if (ch == '1') {
      startGame();
    }
    else if (ch == '2') {
      continue;
    }
    else if (ch == '3') {
      printLeaderboard();
    }
    else if (ch == '4') {
      clear();
      endwin();
      return 0;
    }
  }
}