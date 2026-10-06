# llama-monitor integration builds

This branch starts at Apollo 0.4.6 commit
`0cd32abaaa141d262477d039ac447b38fe99c394`. The upstream `master` branch is
unchanged. Changes are reviewed here before deployment; upstream updates are
merged explicitly and require a new validation cycle.
The custom branch is the fork's default, so its workflow can be dispatched on
demand. PRs target the separate `integration` branch; no PR targets upstream or
modifies the preserved `master` branch.

## Authentication

Each successful JSON `POST /api/login` creates an independent session. Existing
browser and monitor sessions remain valid. Salted cookie hashes are held only in
memory, with a 30-day absolute lifetime and a maximum of 256 live sessions.
Expired sessions are removed on access. A full store returns HTTP 503 without a
new cookie or eviction. Failed credentials do not modify the store. Changing the
password invalidates all sessions; restarting Apollo drops them as well.

`GET /api/configLocale` adds `auth_sessions: "multiple-v1"`. A monitor must
check this before every login, including renewals. Other API contracts, origin
checks, cookie attributes, streaming hooks, and pairing remain unchanged.
Cookie headers and invalid login bodies are redacted from debug logs.

## Build and test

The **Windows integration build** workflow builds the pinned submodules with
MSYS2 UCRT64, runs the production session-store GoogleTests (including concurrent
login/poll simulation), then produces a portable ZIP. Run it from this branch,
or push changes to trigger it. Each artifact includes its source commit, package
SHA-256, submodule revisions, and test results. This is a custom unsigned build,
not an official Apollo release. Its version includes `llama-auth` and the commit.
The build checks whether the Windows SDK already declares the synthetic pointer
API, keeping Apollo's fallback declarations only when absent.

Local builds use the dependencies and commands in `docs/building.md`.
To run just the session-store tests without the graphics stack:

```sh
g++ -std=c++23 -pthread -I. -Ithird-party/googletest/googletest/include \
  -Ithird-party/googletest/googletest tests/unit/test_auth_sessions.cpp \
  third-party/googletest/googletest/src/gtest-all.cc \
  third-party/googletest/googletest/src/gtest_main.cc -o test_auth_sessions
./test_auth_sessions
```

## Deployment and rollback

Use the companion llama-monitor `scripts/apollo-build.ps1` installer after
verifying the artifact checksum. End streams and explicitly confirm disconnection;
the installer rejects any connected clients reported by the monitor,
backs up installed program files and configuration, and replaces program files
only. It does not install drivers, change pairing, install hooks, or enable AI
switching. Rollback restores the original program files and preserves current
configuration; the original configuration backup remains available for recovery.

Before calling the build ready, test repeated browser logins while the monitor
polls, Desktop and Steam Big Picture, multiple clients, reconnects during model
loading, and Apollo/backend restarts. Confirm the original model invocations and
both AI endpoints return after final disconnect. Do not auto-update installed
integration builds. No upstream contribution is made without explicit approval.
