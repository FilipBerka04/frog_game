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

//************************************************
//*********************ENUMS**********************
//************************************************





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
  border_t *border;
}map_t;


typedef struct {
  position_t position;
  const int delayBetweenMoves;
  int regeneration;
}frog_t;


typedef struct {

}badCar_t;


typedef struct {

}goodCar_t;


typedef struct {

}stork_t;

//************************************************
//******************INITIALIZORS******************
//************************************************

map_t* initMap(int mapWidth, int mapHeight, int x, int y){
  map_t* map = (map_t*)malloc(sizeof(map_t));
  map->width = mapWidth;
  map->height = mapHeight;
  map->position = (position_t){x, y};
  map->border = (border_t*)malloc(sizeof(border_t));
  map->border->borderChar = '#';
  map->border->position = (position_t){x-1, y-1};
  return map;
}


//************************************************
//*******************FUNCTIONS********************
//************************************************

void printBorder(const map_t* map){
  for(int x = 0; x < (map->width) + 2; x++){
    mvaddch(map->border->position.y, map->border->position.x + x, map->border->borderChar);
    mvaddch(map->border->position.y + map->height + 1, map->border->position.x + x, map->border->borderChar);
  }
  for(int y = 0; y < map->height; y++){
    mvaddch(map->border->position.y + y + 1, map->border->position.x, map->border->borderChar);
    mvaddch(map->border->position.y + y + 1, map->border->position.x + map->width + 1, map->border->borderChar);
  }
}


void printMap(const map_t* map){
  printBorder(map);
}

void printPlayer(const frog_t player, const map_t* map){
  mvaddch(player.position.y + map->position.y, player.position.x + map->position.x, 'F');
}


void printScreen(const frog_t player, const map_t* map){
  clear();
  printPlayer(player, map);
  printMap(map);
  refresh();
}


void movePlayer(frog_t* player, position_t move){
  if(player->regeneration)
    return;
  player->position.x = player->position.x + move.x;
  player->position.y = player->position.y + move.y;
  player->regeneration = player->delayBetweenMoves;
}

void regeneratePlayer(frog_t* player){
  if(player->regeneration > 0)
    player->regeneration -= 1;
}


void input(char ch, frog_t* player, char* stop) {
  switch (ch) {
    case 'w':
      movePlayer(player, UP);
      break;
    case 's':
      movePlayer(player, DOWN);
      break;
    case 'a':
      movePlayer(player, LEFT);
      break;
    case 'd':
      movePlayer(player, RIGHT);
      break;
    case 'q':
      *stop = 1;
      break;
  }
}





int main() {
  initscr();
  clear();
  noecho();
  nodelay(stdscr, 1);
  curs_set(0);
  struct timespec ts = {0, 10000000};

  map_t *map = initMap(31, 40, 1, COLS/2 - 31/2);
  frog_t player = { .position = {38,16}, .delayBetweenMoves = 30, .regeneration = 0 };
  char stop = 0;
  while(!stop){
    input(getch(), &player, &stop);

    printScreen(player, map);
    regeneratePlayer(&player);
    nanosleep(&ts, NULL);
  }
  clear();
  endwin();
  return 0;
}