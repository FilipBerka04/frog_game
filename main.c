#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>

//************************************************
//*******************DEFINES**********************
//************************************************

#define UP (position_t){ -1, 0 }
#define DOWN (position_t){ 1, 0 }
#define LEFT (position_t){ 0, -1 }
#define RIGHT (position_t){ 0, 1 }

#define MAP_WIDTH 31
#define MAP_HEIGHT 40
#define MAP_POSITION ((position_t){ 1, COLS/2 - 31/2 })

//************************************************
//*********************ENUMS**********************
//************************************************

typedef enum {
  GRASS,
  ROAD_LR,
  ROAD_RL,
}lane_t;


typedef enum {
  BOUNCING,
  WRAPPING,
  DISAPPEARING,
}carBehaviour_t;


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
  carBehaviour_t carBehaviour;
}car_t;


typedef struct {

}stork_t;


typedef struct {
  map_t map;
  player_t player;
  car_t* cars;
  char stop;
}game_t;

//************************************************
//**********************INITS*********************
//************************************************


lane_t* initLanes(map_t *map) {
  lane_t *lanes = (lane_t*)malloc(sizeof(lane_t) * map->height);
  lanes[0] = lanes[map->height - 1] = GRASS;
  for (int i = 1; i < map->height-1; i++) {
    if (!(i%4)) {
      lanes[i] = ROAD_LR;
      map->roadCount++;
    }
    else
    if (!(i%7)) {
      lanes[i] = ROAD_RL;
      map->roadCount++;
    }
    else
      lanes[i] = GRASS;
  }
  return lanes;
}


car_t* initCars(const int numCars) {
  car_t *cars = (car_t*)malloc(sizeof(car_t) * numCars);
  for (int i = 0; i < numCars; i++) {
    cars[i] = (car_t){
      .position = MAP_POSITION,
      .good = 0,
      .length = 1,
      .regeneration = 0,
      .carBehaviour = WRAPPING,
      .delayBetweenMoves = 30
    };
  }
  return cars;
}

map_t initMap(const int mapWidth, const int mapHeight, const position_t position){
  map_t map;
  map.width = mapWidth;
  map.height = mapHeight;
  map.position = position;
  map.border.borderChar = '#';
  map.border.position = (position_t){position.y-1, position.x-1};
  map.lanes = initLanes(&map);
  return map;
}


player_t initPlayer(const position_t position, const int delay) {
  return (player_t){ .position = position, .delayBetweenMoves = delay, .regeneration = 0 };
}


game_t initGame() {
  game_t game;
  game.map = initMap(MAP_WIDTH, MAP_HEIGHT, MAP_POSITION);
  game.player = initPlayer( (position_t){MAP_HEIGHT-1, MAP_WIDTH/2}, 0);
  game.cars = initCars(5);
  game.stop = 0;
  return game;
}

//************************************************
//*******************FUNCTIONS********************
//************************************************

//-----------------printing stuff-----------------
void printBorder(const map_t map){
  for(int x = 0; x < (map.width) + 2; x++){
    mvaddch(map.border.position.y, map.border.position.x + x, map.border.borderChar);
    mvaddch(map.border.position.y + map.height + 1, map.border.position.x + x, map.border.borderChar);
  }
  for(int y = 0; y < map.height; y++){
    mvaddch(map.border.position.y + y + 1, map.border.position.x, map.border.borderChar);
    mvaddch(map.border.position.y + y + 1, map.border.position.x + map.width + 1, map.border.borderChar);
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
  printBorder(map);
  for(int y = 0; y < map.height; y++) {
    printLane((position_t){ map.position.y + y, map.position.x}, map.lanes[y], map.width);
  }
}

void printPlayer(const game_t* game){
  mvaddch(game->player.position.y + game->map.position.y, game->player.position.x + game->map.position.x, 'O');
}


void printGame(const game_t *game){
  clear();
  printMap(game->map);
  printPlayer(game);
}


//-----------------control stuff------------------
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


void regeneratePlayer(player_t* player){
  if(player->regeneration > 0)
    player->regeneration -= 1;
}


void input(const char key, game_t *game) {
  switch (key) {
    case 'w':
      movePlayer(game, UP);
      break;
    case 's':
      movePlayer(game, DOWN);
      break;
    case 'a':
      movePlayer(game, LEFT);
      break;
    case 'd':
      movePlayer(game, RIGHT);
      break;
    case 'q':
      game->stop = 1;
      break;
    default:;
  }
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
  const struct timespec ts = {0, 10000000};

  game_t game = initGame();
  while(!game.stop) {
    input(getch(), &game);

    printGame(&game);
    regeneratePlayer(&game.player);
    nanosleep(&ts, NULL);
  }
  freeGame(&game);
  clear();
  endwin();
  return 0;
}