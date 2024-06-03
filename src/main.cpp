#include <chrono>
#include <functional>
#include <unistd.h>

#include "Game.hpp"
#include "utils.hpp"

using namespace std;
using namespace utils;

int getDifficultyLevel(void);
void mloop(function<void(void)> op);

int main(void) {
  Game game = Game(getDifficultyLevel());
  mloop([&game]() { game.update(); });

  return 0;
}

int getDifficultyLevel(void) {
  int d;

  std::cout << "Select the difficulty level:" << std::endl;
  std::cout << "1 - Fácil" << std::endl;
  std::cout << "2 - Médio" << std::endl;
  std::cout << "3 - Difícil" << std::endl;
  std::cin >> d;
  if (d <= 0 || d > 3) {
    std::cout << "Select a value between 1 and 3." << std::endl;
    exit(1);
  }

  return d;
}

void mloop(function<void(void)> op) {
  auto now = chrono::system_clock::now();
  auto lastFrame = chrono::system_clock::now();

  while (true) {
    now = chrono::system_clock::now();
    const int delta = (now - lastFrame).count() / FR_MODIFIER;
    lastFrame = now;

    if (delta < (FR * FR_MODIFIER)) {
      usleep((FR * FR_MODIFIER) - delta);
    }

    op();
  }
}
