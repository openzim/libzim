#ifndef ZIM_LOCK_H
#define ZIM_LOCK_H

#include <algorithm>
#include <mutex>
#include <system_error>
#include <vector>

class MultiMutex {
  public:
    explicit MultiMutex()
     : m_mutexes()
     {}
    explicit MultiMutex(const std::vector<std::recursive_mutex*>& mutexes)
     : m_mutexes(mutexes) {
    // By sorting the mutex, we avoid the simple case when 3 concurrent multi lock:
    // - (A, B)
    // - (B, C)
    // - (C, A)
    // As we sort, we will have :
    // - (A, B)
    // - (B, C)
    // - (A, C)
    //
    // And no deadlock can occurs.
       std::sort(m_mutexes.begin(), m_mutexes.end());
     }

    void lock() {
      auto lockedCount = 0;
      for (auto mutex:m_mutexes) {
        try {
          mutex->lock();
          lockedCount += 1;
        } catch(const std::system_error& e) {
          unwindLock(lockedCount);
          throw;
        }
      }
    }

    void unlock() {
      unwindLock(m_mutexes.size());
    }


  private:
    std::vector<std::recursive_mutex*> m_mutexes;

    void unwindLock(size_t lockedCount) {
      while (lockedCount) {
        m_mutexes[--lockedCount]->unlock();
      }
    }
};

#endif // ZIM_LOCK_H
