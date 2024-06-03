#include "Entity.hpp"

namespace Entity {
  namespace Sync {
    // entra na seção crítica para escrita
    void enterWriterCriticalSection(utils::Types::CriticalResource *resource) {
      sem_wait(&resource->writerSem);
      resource->writeCount++;
      if (resource->writeCount == 1)
        sem_wait(&resource->readTrySem);
      sem_post(&resource->writerSem);
      sem_wait(&resource->resourceSem);
    }

    // sai da seção crítica para escrita
    void exitWriterCriticalSection(utils::Types::CriticalResource *resource) {
      sem_post(&resource->resourceSem);
      sem_wait(&resource->writerSem);
      resource->writeCount--;
      if (resource->writeCount == 0)
        sem_post(&resource->readTrySem);
      sem_post(&resource->writerSem);
    }

    // executa uma operação de escrita na seção crítica
    void performWriteOperation(utils::Types::CriticalResource *resource, std::function<void(void)> op) {
      enterWriterCriticalSection(resource);
      op();
      exitWriterCriticalSection(resource);
    }

    // entra na seção crítica como leitura
    utils::Types::CriticalResource readerEnterCSection(utils::Types::CriticalResource *resource) {
      sem_wait(&resource->readTrySem);
      sem_wait(&resource->readerSem);
      resource->readCount++;
      if (resource->readCount == 1)
        sem_wait(&resource->resourceSem);
      sem_post(&resource->readerSem);
      sem_post(&resource->readTrySem);
      return *resource;
    }

    // sai da seção crítica para leitura
    void exitReaderCriticalSection(utils::Types::CriticalResource *resource) {
      sem_wait(&resource->readerSem);
      resource->readCount--;
      if (resource->readCount == 0)
        sem_post(&resource->resourceSem);
      sem_post(&resource->readerSem);
    }
    // executa uma operação de leitura na seção crítica
    void performReadOperation(utils::Types::CriticalResource *resource,
                          std::function<void(utils::Types::CriticalResource resource)> op) {
      op(readerEnterCSection(resource));
      exitReaderCriticalSection(resource);
    }
  }
  // lançar míssil
  namespace {
    void launchMissile(utils::Types::GameState *state, utils::Types::MissileProps *props) {
      bool empty;
      Sync::performWriteOperation(state->battery, [&props, &state, &empty]() {
        if (state->battery->n > 0) {
          state->battery->n--;
          empty = false;
        } else {
          empty = true;
        }
      });
      if (!empty) {
        pthread_t thread;
        pthread_create(&thread, NULL, missileThread, props);
      }
    }
  }        
  // Thread do jogador      
  void *playerThread(void *arg) { 
    utils::Types::GameState *state = (utils::Types::GameState *)arg;
    utils::Types::Board &board = state->boardState;
    int &pos = state->playerPosition;

    int x, y;
    y = state->boardState.size();
    x = state->boardState[0].size();
    std::vector<utils::Types::Element *> &playerRow = board[y - 1];
    pos = x / 2;

    while (true) {
      int ch;
      int newPos;
      if ((ch = getch()) != ERR) {
        if (ch >= 'A' && ch <= 'Z')
          ch += 32;
        switch (ch) {
        case 'd':
          newPos = pos + 1;
          break;
        case 'a':
          newPos = pos - 1;
          break;
        case ' ':
          utils::Types::MissileProps *props = new utils::Types::MissileProps;
          props->state = state;
          props->playerPosX = pos;
          launchMissile(state, props);
        }
      }
      if (newPos != pos && newPos > 0 && newPos < x) {
        playerRow[pos]->displayValue = 0;
        pos = newPos;
      }
      playerRow[pos]->displayValue = utils::Types::EntityEnum::PLAYER;
      usleep(utils::INPUT_INTERVAL);
    }
  }
  // Thread do alien
  void *alienThread(void *arg) {
    int id;
    utils::Types::GameState *state;
    utils::Types::Alien *self;
    std::pair<int, int> area;

    utils::Types::AlienProps *props = (utils::Types::AlienProps *)arg;

    state = props->state;
    id = props->id;
    self = state->aliens[id];
    area = props->playableArea;
    bool alive = true;
    auto maxY = area.first;

    delete props;

    while (true) {
      auto pos = self->pos;
      auto &board = state->boardState;
      auto x = pos.first;
      auto y = pos.second;

      Sync::performWriteOperation(board[x][y], [&state, &self, &alive, &y, x, maxY, id]() {
        state->boardState[x][y]->displayValue = 0;
        Sync::performWriteOperation(self, [&state, &self, &alive, &y, x, maxY, id]() {
          alive = self->alive;
          if (!alive)
            return;
          auto nextY = (y + 1) % maxY;
          Sync::performWriteOperation(state->boardState[x][nextY], [&state, &self, x, y, nextY, id]() {
            if (state->boardState[x][nextY]->displayValue == 0) {
              state->boardState[x][nextY]->displayValue = utils::Types::EntityEnum::ENEMY;
              state->boardState[x][nextY]->entityId = id;
              state->boardState[x][y]->entityId = 0;
              self->pos = {x, nextY};
              return;
            }
            state->boardState[x][nextY]->displayValue = 0;
            state->boardState[x][nextY]->entityId = 0;
            state->boardState[x][y]->displayValue = utils::Types::EntityEnum::ENEMY;
          });
        });
      });
      usleep(utils::ENEMY_MOV_SPEED_FACT / state->difficulty);
      if (!alive)
        break;
    }

    return NULL;
  }

  // Thread do míssil
  void *missileThread(void *arg) {
    utils::Types::MissileProps *props = (utils::Types::MissileProps *)arg;
    utils::Types::GameState *state = props->state;
    std::pair<int, int> initialPos = {state->boardState.size() - 2, props->playerPosX};
    int prev;

    delete props;

    for (int i = initialPos.first; i >= 0; i--) {
      bool hit = false;

      if (prev) {
        Sync::performWriteOperation(state->boardState[prev][initialPos.second], [&state, prev, initialPos]() {
          state->boardState[prev][initialPos.second]->displayValue = 0;
        });
      }
      Sync::performWriteOperation(state->boardState[i][initialPos.second], [&state, &hit, i, initialPos]() {
        if (state->boardState[i][initialPos.second]->displayValue == 2) {
          auto alien = state->aliens[state->boardState[i][initialPos.second]->entityId];
          Sync::performWriteOperation(alien, [&alien]() { alien->alive = false; });
          hit = true;
        }
        state->boardState[i][initialPos.second]->displayValue = 3;
        hit = false;
      });
      if (hit)
        break;
      prev = i;
      usleep(utils::MISSILE_MOV_SPEED_FACT);
    }
    state->boardState[prev][initialPos.second]->displayValue = 0;

    return NULL;
  }

  // Thread do gerador de mísseis
  void *missileGeneratorThread(void *arg) {
    utils::Types::GameState *state = (utils::Types::GameState *)arg;
    while (true) {
      Sync::performWriteOperation(state->battery, [&state]() {
        if (state->battery->n < utils::MAX_MISSILE_CAPACITY) {
          state->battery->n++;
        }
      });
      sleep(utils::MISSILE_GENERATOR_INTERVAL);
    }
  }

  // Thread do temporizador do jogo
  void *gameTimerThread(void *arg) {
    utils::Types::GameState *state = (utils::Types::GameState *)arg;
    int timePast = 0;

    while (true) {
      int won = 1;
      for (auto alien : state->aliens) {
        if (alien->alive)
          won = 0;
      }
      if (timePast >= utils::TIME_LIMIT)
        won = -1;
      state->over = won;
      if (state->over != 0)
        break;
      timePast++;
      state->timePast = timePast;
      sleep(1);
    }

    return NULL;
  }
}
