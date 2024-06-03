#ifndef __ENTITY_H__
#define __ENTITY_H__

#include <functional>
#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <unistd.h>
#include <utility>

#include "utils.hpp"

namespace Entity {
  namespace Sync {
    void enterWriterCriticalSection(utils::Types::CriticalResource *resource);
    void exitWriterCriticalSection(utils::Types::CriticalResource *resource);
    void performWriteOperation(utils::Types::CriticalResource *resource, std::function<void()> op);
    utils::Types::CriticalResource readerEnterCSection(utils::Types::CriticalResource *resource);
    void exitReaderCriticalSection(utils::Types::CriticalResource *resource);
    void performReadOperation(utils::Types::CriticalResource *resource, std::function<void(utils::Types::CriticalResource resource)> op);
  }
  void *playerThread(void *arg);
  void *missileThread(void *arg);
  void *alienThread(void *arg);
  void *gameTimerThread(void *arg);
  void *missileGeneratorThread(void *arg);
} 

#endif
