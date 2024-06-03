#ifndef __GAME_H__
#define __GAME_H__

#include <exception>
#include <iostream>
#include <math.h>
#include <ncurses.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "Entity.hpp"
#include "PlayerInterface.hpp"
#include "utils.hpp"

class Game {
  utils::Types::GameState *state;
  PlayerInterface interface;

  void checkMinimumDimensions(int x, int y);
  void checkOverState(void);
  void constructGameStructures(int x, int y);
  void startGameThreads(void);

public:
  Game(int diff);
  ~Game(void);

  void update(void);
};

#endif
