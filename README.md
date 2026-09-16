<div align="center">

<img width="2172" height="724" alt="WEB HTOP" src="https://github.com/user-attachments/assets/41763a10-4e2e-4e10-9991-30f218d654c0" />

### Collect once. Observe everywhere. Replay what happened.
One Linux sampler, many independent observers, and no screenshot archaeology.

<p>
  <img src="https://img.shields.io/badge/C%2B%2B-26-3f4550?style=flat&logo=cplusplus&logoColor=c9d1d9" alt="C++26">
  <img src="https://img.shields.io/badge/platform-Linux-3f4550?style=flat&logo=linux&logoColor=c9d1d9" alt="Linux">
  <a href="https://github.com/RomanSnitko/web_htop/actions/workflows/ci.yml"><img src="https://img.shields.io/badge/CI-GCC%20%7C%20Clang-3f4550?style=flat&logo=githubactions&logoColor=c9d1d9" alt="CI"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-3f4550?style=flat" alt="MIT"></a>
</p>

</div>

WEB HTOP is a telemetry server and terminal client for inspecting one
Linux host from several terminals, scripts, or monitoring systems at the same
time. The host is sampled once; complete immutable generations are then fanned
out over TCP and HTTP.

It is not htop with a socket bolted onto it. Collection, publication,
transport, rendering, and replay have separate ownership and failure domains.

```cpp
// architecture pseudocode :)
web_htop
{
    host
    {
        // metrics collection
        sampler[jthread]
        {
            procfs + sysfs + PSI + cgroup_v2;
            zero_allocation_parsers -> generation<N>;
        }

        // transfer a snapsh. between threads
        publication[SPMC] // engineering flex
        {
            immutable_snapshots;
            lock_free_handoff;
            hazard_pointer_reclamation;
            pool_allocator;
            encode_once;
        }

        // network part
        reactor[epoll]
        {
            eventfd + timerfd + signalfd;
            TCP  -> framed_JSON -> observers[*];
            HTTP -> { health, ready, metrics, processes, diagnostics, exporter };
            slow_reader -> { current_frame, latest_pending_frame };
        }
    }

    // consumers
    observer[*]
    {
        TUI | curl | Prometheus | record(JSONL) -> replay;
    }
}
```

## The engineering parts

- Lock-free publication. A single producer publishes immutable generations
  to multiple consumers through a bounded SPMC path. Acquire/release ordering
  defines visibility; hazard pointers reclaim retired snapshots without moving
  lifetime management onto the hot path.
- Controlled allocation. A pool allocator backs short-lived publication and
  transport objects. The procfs/loadavg parsers use string_view and avoid heap
  allocation in their steady-state paths.
- JSON owned by the protocol. The parser and serializer are written in-tree:
  UTF-16 surrogate pairs, typed numeric limits, owned strings, depth/node/input
  limits, duplicate-key rejection, invalid UTF-8 rejection, and non-throwing
  failures through std::optional.
- Backpressure with an opinion. Telemetry is latest-state data, not an audit
  log. A partially written frame is completed; an unsent pending frame may be
  replaced by a newer one. Slow readers lose stale generations, not everyone
  else's latency or the server's memory.
- Linux-native I/O. One level-triggered epoll owner manages listeners,
  sessions, deadlines, and diagnostics. Descriptor reuse is guarded by session
  tokens; UniqueFd and RAII own every exit path.
- Failure is data. warming_up, partial, unavailable, and stale are
  different states. Collector duration, snapshot age, queue pressure, dropped
  generations, reconnects, and timeouts are observable.
- Incidents survive the terminal. The client can append received generations
  to JSONL and replay them later without a running server.

## Run it

Requirements: Linux or WSL2, CMake 3.20+, and a C++20 compiler.

```bash
git clone https://github.com/RomanSnitko/web_htop.git
cd web_htop

cmake --preset release
cmake --build --preset release -j"$(nproc)"
ctest --test-dir build-release --output-on-failure --no-tests=error
```

```bash
# terminal 1: collect once
./build-release/server/web_htop_server

# terminal 2..N: observe independently
./build-release/client/web_htop_client localhost 9999 8080
```

The server binds to 127.0.0.1 by default. For another machine, forward ports
9999 and 8080 over SSH rather than exposing an unauthenticated telemetry
endpoint to the internet. The penguin is friendly; the network is not.

<details>
<summary><strong>Record, replay, and one-shot output</strong></summary>

```bash
./build-release/client/web_htop_client localhost 9999 8080 --record incident.jsonl
./build-release/client/web_htop_client --replay incident.jsonl
./build-release/client/web_htop_client localhost 9999 8080 --once
```

Space freezes only the visible generation. Acquisition and recording continue.

</details>

<details>
<summary><strong>Wire and HTTP surface</strong></summary>

TCP messages are a four-byte big-endian length followed by an owned UTF-8 JSON
document, capped at 8 MiB. Every frame is self-contained and carries a protocol
version, server instance ID, generation sequence, timestamps, and section status.

```bash
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/ready
curl http://127.0.0.1:8080/metrics
curl http://127.0.0.1:8080/processes
curl http://127.0.0.1:8080/diagnostics
curl http://127.0.0.1:8080/exporter
```

</details>

<details>
<summary><strong>Break it before users do</strong></summary>

The suite covers fragmented/coalesced frames, partial writes, stalled readers,
PID reuse, counter resets, queue replacement, descriptor reclamation, concurrent
publication, shutdown with pending output, parser limits, and malformed input.

```bash
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan
```

CI builds with GCC and Clang, runs ASan/UBSan/TSan, CodeQL, integration tests,
and short libFuzzer campaigns against JSON, TCP framing, and procfs parsing.

</details>

## Design rules

```text
one owner per mutable resource
publish only complete generations
encode once, share immutable bytes
bound every network-controlled buffer
never let a slow observer stall collection
measure before replacing a boring primitive with a clever one
```

Contributions are welcome, especially around Linux collectors, concurrency,
network failure modes, benchmarks, and terminal UX. For non-trivial changes,
open an issue first and bring either a reproducer or measurements.
