/* ps2_port.h: included before every C++ source of the PS2 build (-include).

   libc++ is built without threads (as for the other ports): it has
   lock_guard and unique_lock but no std::mutex or std::scoped_lock. Those two
   are here, on maxsdk's semaphores. A mutex's semaphore is made at its first
   lock, so static mutexes work before main. */
#ifndef PS2_PORT_H
#define PS2_PORT_H

#ifdef __cplusplus
#include <__config>

extern "C" {
int mx_sema_create(int init, int max);
void mx_sema_delete(int s);
void mx_sema_wait(int s);
void mx_sema_signal(int s);
int mx_sema_try(int s);
}

_LIBCPP_BEGIN_NAMESPACE_STD

class mutex {
  int sema_ = -1;
  int sema() {
    if (sema_ < 0)
      sema_ = mx_sema_create(1, 1);
    return sema_;
  }

public:
  constexpr mutex() noexcept = default;
  mutex(const mutex &) = delete;
  mutex &operator=(const mutex &) = delete;
  ~mutex() {
    if (sema_ >= 0)
      mx_sema_delete(sema_);
  }
  void lock() { mx_sema_wait(sema()); }
  bool try_lock() { return mx_sema_try(sema()) != 0; }
  void unlock() { mx_sema_signal(sema()); }
};

template <class... M> class scoped_lock;
template <class M> class scoped_lock<M> {
  M &m_;

public:
  explicit scoped_lock(M &m) : m_(m) { m_.lock(); }
  ~scoped_lock() { m_.unlock(); }
  scoped_lock(const scoped_lock &) = delete;
  scoped_lock &operator=(const scoped_lock &) = delete;
};

template <class... M> scoped_lock(M &...) -> scoped_lock<M...>;

_LIBCPP_END_NAMESPACE_STD
#endif

#endif
