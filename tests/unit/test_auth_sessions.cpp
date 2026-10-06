#include "src/auth_sessions.h"
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>

using confighttp::auth_sessions_t;

TEST(WebAuthSessions, IndependentSessionsAndUnknownHashes) {
  auth_sessions_t sessions;
  EXPECT_TRUE(sessions.insert("browser-hash"));
  EXPECT_TRUE(sessions.insert("monitor-hash"));
  EXPECT_TRUE(sessions.contains("browser-hash"));
  EXPECT_TRUE(sessions.contains("monitor-hash"));
  EXPECT_FALSE(sessions.contains("unknown-hash"));
  EXPECT_FALSE(sessions.insert(""));
}

TEST(WebAuthSessions, IndependentAbsoluteExpiration) {
  auth_sessions_t sessions;
  const auto start = auth_sessions_t::clock::time_point{};
  EXPECT_TRUE(sessions.insert("browser", start));
  EXPECT_TRUE(sessions.insert("monitor", start + std::chrono::hours(24)));
  EXPECT_TRUE(sessions.contains("browser", start + auth_sessions_t::lifetime - std::chrono::seconds(1)));
  EXPECT_FALSE(sessions.contains("browser", start + auth_sessions_t::lifetime));
  EXPECT_TRUE(sessions.contains("monitor", start + auth_sessions_t::lifetime));
}

TEST(WebAuthSessions, CapacityNeverEvictsAndExpiredEntriesFreeSpace) {
  auth_sessions_t sessions;
  const auto start = auth_sessions_t::clock::time_point{};
  for (std::size_t i = 0; i < sessions.capacity; ++i) {
    ASSERT_TRUE(sessions.insert(std::to_string(i), start));
  }
  EXPECT_FALSE(sessions.insert("extra", start));
  for (std::size_t i = 0; i < sessions.capacity; ++i) {
    EXPECT_TRUE(sessions.contains(std::to_string(i), start));
  }
  EXPECT_TRUE(sessions.insert("extra", start + sessions.lifetime));
}

TEST(WebAuthSessions, CollisionDoesNotExtendExistingSession) {
  auth_sessions_t sessions;
  const auto start = auth_sessions_t::clock::time_point{};
  EXPECT_TRUE(sessions.insert("same", start));
  EXPECT_FALSE(sessions.insert("same", start + std::chrono::hours(1)));
  EXPECT_FALSE(sessions.contains("same", start + sessions.lifetime));
}

TEST(WebAuthSessions, RevocationAndRestartDiscardEverySession) {
  auth_sessions_t sessions;
  EXPECT_TRUE(sessions.insert("browser"));
  EXPECT_TRUE(sessions.insert("monitor"));
  sessions.clear();
  EXPECT_FALSE(sessions.contains("browser"));
  EXPECT_FALSE(sessions.contains("monitor"));
  EXPECT_FALSE(auth_sessions_t{}.contains("browser"));
}

TEST(WebAuthSessions, ConcurrentLoginsAndPollsPreserveExistingSessions) {
  auth_sessions_t sessions;
  ASSERT_TRUE(sessions.insert("browser"));
  ASSERT_TRUE(sessions.insert("monitor"));
  std::atomic_bool success = true;
  std::vector<std::thread> workers;
  for (int worker = 0; worker < 8; ++worker) {
    workers.emplace_back([&, worker] {
      for (int i = 0; i < 20; ++i) {
        if (!sessions.insert(std::to_string(worker) + ":" + std::to_string(i)) ||
            !sessions.contains("browser") || !sessions.contains("monitor")) {
          success = false;
        }
      }
    });
  }
  for (auto &worker : workers) { worker.join(); }
  EXPECT_TRUE(success);
}
