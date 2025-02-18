#ifndef ZIM_LOCK_H
#define ZIM_LOCK_H

#include <memory>
#include <mutex>

class MultiMutex {
  public:
    explicit MultiMutex(std::recursive_mutex* m1)
     : m1(m1), m2(nullptr) {}
    MultiMutex(std::recursive_mutex* m1, std::unique_ptr<MultiMutex> m2)
     : m1(m1), m2(std::move(m2)) {}

    template<typename Iterator>
    static std::unique_ptr<MultiMutex> create(Iterator begin, Iterator end) {
      if (begin == end) {
        return nullptr;
      }

      auto second = begin;
      second++;
      if (second == end) {
        return std::make_unique<MultiMutex>(*begin);
      }

      return std::make_unique<MultiMutex>(*begin, MultiMutex::create(second, end));
    }

    void lock() {
      if (m2) {
        std::lock(*m1, *m2);
      } else {
        m1->lock();
      }
    }

    void unlock() {
      m1->unlock();
      if (m2) m2->unlock();
    }

    bool try_lock() {
      if (m2) {
        return std::try_lock(*m1, *m2);
      } else {
        return m1->try_lock();
      }
    }

  private:
    std::recursive_mutex* m1;
    std::unique_ptr<MultiMutex> m2;
};

#endif // ZIM_LOCK_H
