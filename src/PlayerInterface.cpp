#include "PlayerInterface.hpp"

using namespace std;

PlayerInterface::PlayerInterface(void) : SCALE(utils::SCALE), COLOR(has_colors()) {}

void PlayerInterface::start(void) {
  initscr();
  curs_set(false);
  cbreak();
  noecho();
  nodelay(stdscr, true);
  keypad(stdscr, TRUE);
  use_default_colors();
  const pair<int, int> d = getDimensions();
  playableArea = {(d.first / (SCALE)), (d.second / (SCALE))};
  if (COLOR) {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
  }
}

void PlayerInterface::mainUpdate(utils::Types::GameState *state) {
  clear();
  draw(state);
  refresh();
}

std::pair<int, int> PlayerInterface::getDimensions(void) {
  int x, y;
  getmaxyx(stdscr, y, x);

  return {x, y};
}

int PlayerInterface::getScaleFactor(void) { return SCALE; }

std::pair<int, int> PlayerInterface::virtualPositionToTerminalCoordinates(std::pair<int, int> pos) {
  return {pos.first * SCALE, pos.second * SCALE};
}

pair<int, int> PlayerInterface::getPlayableArea(void) { return playableArea; }

vector<string> PlayerInterface::getSprite(int e) {
  switch (e) {
  case utils::Types::EntityEnum::ENEMY:
    return {"/W\\", "V-V", "   "};
    break;
  case utils::Types::EntityEnum::PLAYER:
    return {" ^ ", " ^ ", " ^ "};
    break;
  case utils::Types::EntityEnum::MISSILE:
    return {"   ", " 0 ", "   "};
    break;
  default:
    return {"   ", "   ", "   "};
  }
}

void PlayerInterface::printBlock(pair<int, int> &pos, vector<string> &sprite) {
  for (int i = 0; i < (int)sprite.size(); i++) {
    mvprintw(pos.second + i, pos.first, sprite[i].c_str());
  }
}

void PlayerInterface::printTimer(utils::Types::GameState *state) {
  map<int, std::pair<int, int>> positionMap{
      {0, {0, 0}},
      {1, {0, (playableArea.second - 2)}},
      {2, {playableArea.first - 1, 0}},
      {3, {playableArea.first - 1, playableArea.second - 2}},
  };
  pair<int, int> virtualPos = positionMap[utils::TIMER_POS];
  pair<int, int> pos = virtualPositionToTerminalCoordinates(virtualPos);

  ostringstream oss;
  oss << internal << setfill('0') << setw(SCALE) << (utils::TIME_LIMIT - state->timePast);
  string timeString = oss.str();
  vector<string> clockSprite = {"   ", timeString, "   "};

  printBlock(pos, clockSprite);
}

void PlayerInterface::printMissileBattery(utils::Types::GameState *state) {
  map<int, std::pair<int, int>> positionMap{
      {0, {0, 0}},
      {1, {0, (playableArea.second - 2)}},
      {2, {playableArea.first - 1, 0}},
      {3, {playableArea.first - 1, playableArea.second - 2}},
  };
  pair<int, int> virtualPos = positionMap[utils::AMMO_COUNTER_POS];
  pair<int, int> pos = virtualPositionToTerminalCoordinates(virtualPos);
  vector<string> counterSprite = {"   ", "  " + to_string(state->battery->n), "   "};

  printBlock(pos, counterSprite);
}

void PlayerInterface::printSprite(pair<int, int> pos, int entity) {
  vector<string> sprite = getSprite(entity);
  printBlock(pos, sprite);
}

void PlayerInterface::draw(utils::Types::GameState *state) {
  for (int i = 0; i < (int)state->boardState.size(); i++) {
    for (int j = 0; j < (int)state->boardState[i].size(); j++) {
      auto coords = virtualPositionToTerminalCoordinates({j, i});
      printSprite(coords, state->boardState[i][j]->displayValue);
    }
  }
  printTimer(state);
  printMissileBattery(state);
}

void PlayerInterface::stop(void) { endwin(); }
