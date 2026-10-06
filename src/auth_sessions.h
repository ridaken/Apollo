/** @file src/auth_sessions.h
 *  @brief Bounded, independent Web UI sessions. Only salted cookie hashes belong here.
 */
#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace confighttp {
  class auth_sessions_t {
  public:
    using clock = std::chrono::steady_clock;
    static constexpr auto lifetime = std::chrono::hours(24 * 30);
    static constexpr std::size_t capacity = 256;

    bool insert(const std::string &hash, clock::time_point now = clock::now()) {
      std::lock_guard lock(mutex_);
      prune(now);
      // Never evict or extend another session, even on an unlikely collision.
      if (hash.empty() || sessions_.contains(hash) || sessions_.size() >= capacity) {
        return false;
      }
      sessions_.emplace(hash, now + lifetime);
      return true;
    }

    bool contains(const std::string &hash, clock::time_point now = clock::now()) {
      std::lock_guard lock(mutex_);
      prune(now);
      return sessions_.contains(hash);
    }

    void clear() {
      std::lock_guard lock(mutex_);
      sessions_.clear();
    }

  private:
    void prune(clock::time_point now) {
      std::erase_if(sessions_, [now](const auto &session) { return session.second <= now; });
    }

    std::mutex mutex_;
    std::unordered_map<std::string, clock::time_point> sessions_;
  };
}  // namespace confighttp
