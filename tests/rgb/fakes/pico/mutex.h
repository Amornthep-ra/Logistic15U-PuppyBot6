#pragma once
#include <mutex>
struct mutex_t { std::mutex mutex; };
#define auto_init_mutex(name) static mutex_t name
inline void mutex_enter_blocking(mutex_t *value) { value->mutex.lock(); }
inline void mutex_exit(mutex_t *value) { value->mutex.unlock(); }
