<div align="center">

<img width="2172" height="724" alt="ChatGPT Image 15 сент  2026 г , 16_23_21" src="https://github.com/user-attachments/assets/cd269fb5-0988-4aea-ab27-31f8cb3104cc" />

### One Linux host. Many observers. No screenshot archaeology.

A C++20 telemetry server and terminal console for watching Linux systems live -
locally, over SSH, from several terminals, or through machine-readable APIs.

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/Concurrency-jthread%20%7C%20stop__token%20%7C%20atomics-7B68EE?style=flat-square&logo=cplusplus&logoColor=white" alt="C++ concurrency">
  <img src="https://img.shields.io/badge/I%2FO-epoll-00B8D9?style=flat-square&logo=linux&logoColor=white" alt="epoll">
  <img src="https://img.shields.io/badge/Linux-procfs%20%7C%20PSI-FCC624?style=flat-square&logo=linux&logoColor=black" alt="Linux procfs and PSI">
  <img src="https://img.shields.io/badge/JSON-custom%20parser-292929?style=flat-square&logo=json&logoColor=white" alt="Custom JSON parser">
  <img src="https://img.shields.io/badge/License-MIT-2ea44f?style=flat-square" alt="MIT License">
</p>

<p align="center">
  <a href="https://github.com/RomanSnitko/web_htop/tree/main/tests">
    <img src="https://img.shields.io/badge/C%2B%2B%20tests-GoogleTest%20%7C%20CTest-34A853?style=flat-square&logo=google&logoColor=white" alt="C++ tests">
  </a>
  <a href="https://github.com/RomanSnitko/web_htop/tree/main/tests/integration">
    <img src="https://img.shields.io/badge/Python%20tests-integration%20%7C%20socket-3776AB?style=flat-square&logo=python&logoColor=white" alt="Python integration tests">
  </a>
  <img src="https://img.shields.io/badge/Sanitizers-ASan%20%7C%20UBSan%20%7C%20TSan-A371F7?style=flat-square" alt="Sanitizers">
  <img src="https://img.shields.io/badge/Fuzzing-JSON%20%7C%20TCP%20%7C%20procfs-EF6C00?style=flat-square" alt="Fuzz testing">
  <img src="https://img.shields.io/badge/Container-Docker-2496ED?style=flat-square&logo=docker&logoColor=white" alt="Docker">
  <img src="https://img.shields.io/badge/Deploy-Kubernetes-326CE5?style=flat-square&logo=kubernetes&logoColor=white" alt="Kubernetes">
  <img src="https://img.shields.io/badge/Arch-amd64%20%7C%20arm64-6E7781?style=flat-square" alt="amd64 and arm64">
</p>

[Quick start](#quick-start) · [Remote observation](#one-host-many-observers) · [Architecture](#architecture) · [HTTP API](#http-api) · [Engineering notes](#engineering-notes)

</div>

<p align="center">
  <img width="1549" height="1015" alt="ChatGPT Image 15 сент  2026 г , 15_26_12" src="https://github.com/user-attachments/assets/fb9d4147-264d-49c4-8870-378228354042" />
</p>

## Not htop over TCP

htop is an excellent local process viewer. WEB HTOP solves a different problem:
collect once on the machine being investigated, then let several independent
observers consume the same coherent telemetry stream.

Run the server on a Linux host and connect from another terminal, another machine,
an SSH tunnel, a diagnostic script, or all of them at once. One observer can freeze
the UI or fall behind without stopping collection and without blocking the others.

| | Traditional local monitor | WEB HTOP |
|---|---|---|
| Collection | Tied to one interactive session | Dedicated server-side sampler |
| Observers | One terminal | Multiple independent TCP and HTTP consumers |
| Consistency | Values redraw as they are read | Immutable, versioned generations |
| Slow client | Usually not a concern | Isolated by bounded latest-wins delivery |
| After the incident | Terminal state is gone | JSONL recording and offline replay |
| Automation | Human-oriented output | HTTP, framed JSON and Prometheus exporter |
| Deployment | Local binary | Native, container, or Kubernetes DaemonSet |

### The 02:13 problem

One server is acting strange. Three people SSH into it. One opens a process monitor,
one runs curl, and the third asks for a screenshot. By the time the screenshot
arrives, the spike is gone and everyone has observed a slightly different moment.

WEB HTOP replaces that ritual with one publication stream. The operator watches the
live dashboard, a teammate connects to the same host, and a recorder keeps the
generations that would otherwise disappear. Fewer screenshots; better evidence.

## Why it is different

- One sampler, many readers. Collection cost does not multiply with the number
  of connected dashboards.
- A snapshot is a real boundary. CPU, memory, process, pressure and transport
  sections are assembled before publication; readers never receive a half-updated
  object.
- Freshness beats backlog. A dashboard needs the newest complete state, not
  fifty stale frames. Each session keeps one in-flight frame and one replaceable
  pending frame.
- Pressure is first-class. PSI answers whether work is stalled on CPU, memory,
  or I/O even when utilization alone looks harmless.
- The monitor monitors itself. Collector duration, snapshot age, queue usage,
  superseded frames, reconnects and timeouts are visible through diagnostics.
- Failures have names. warming_up, partial, unavailable, stale and a
  valid numeric zero are not collapsed into the same value.
- Incidents are replayable. Received generations can be recorded as JSONL and
  replayed without a running server.

## One host, many observers

The safe default is loopback-only. For remote use, keep the server on loopback and
forward both interfaces over SSH:

```bash
# On the machine being monitored
./build/server/web_htop_server
```

```bash
# On every observer machine
ssh -N \
  -L 9999:127.0.0.1:9999 \
  -L 8080:127.0.0.1:8080 \
  user@monitored-host
```

```bash
# Each observer gets an independent live console
./build/client/web_htop_client localhost 9999 8080
```

The TCP stream fans out complete snapshots to dashboards. HTTP serves point-in-time
queries and diagnostics. A slow TCP reader cannot hold a mutex needed by another
client or by the collector, and memory remains bounded by per-session and global
queue limits.

WEB HTOP does not implement transport authentication or TLS. Keep the default
loopback binding, use SSH, or place explicitly exposed listeners inside a trusted
network boundary.

## See it in action

| Process Explorer | I/O telemetry |
|---|---|
| <img width="560" alt="WEB HTOP process explorer" src="https://github.com/user-attachments/assets/d75255a6-a791-4dc1-94c2-3da9e6d94886" /> | <img width="560" alt="WEB HTOP I/O workspace" src="https://github.com/user-attachments/assets/5a649b4b-4ee4-4f14-971c-1b6120f85e1b" /> |

The client has six focused workspaces:

| Key | Workspace | What it answers |
|---:|---|---|
| `1` | Overview | Is the host healthy and is telemetry fresh? |
| `2` | Processes | Which processes consume CPU and resident memory? |
| `3` | I/O | Which interfaces and block devices are active or failing? |
| `4` | Pressure | Are tasks stalled, and what does the selected cgroup see? |
| `5` | Transport | Are clients falling behind or being disconnected? |
| `6` | CPU Matrix | How is work distributed across real Linux CPU IDs? |

Use `c/m/p/t` to sort processes, `/` to filter, `j/k` to scroll, `Space` to
freeze the visible generation, and `h` for the complete key map. Freeze affects
presentation only: the network reader continues to drain the stream.

<div align="center">
<img width="320" height="320" alt="web-htop-demo" src="https://github.com/user-attachments/assets/c332cb58-178b-4b89-b56c-b474648c3242" />
</div>

The steady-state server uses two threads with deliberately separate ownership:

- the sampling std::jthread owns collectors and their previous counter values;
- the foreground reactor owns listeners, accepted sockets, session queues and
  network diagnostics.

A generation is assembled, serialized, framed and timestamped before publication.
SharedState release-stores a shared_ptr<const PublishedSnapshot>; readers acquire
one immutable version and keep it alive for the duration of their operation. The
snapshot is encoded once, not once per connected client.

The publication boundary is honest about its limits: Linux metrics are read
sequentially, not atomically by the kernel. Collection start/end timestamps and
per-section durations expose that sampling window.

## Quick start

Requirements: Linux or WSL2, CMake 3.20+, a C++20 compiler, and Python 3 for the
integration tests.

```bash
git clone https://github.com/RomanSnitko/web_htop.git
cd web_htop

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DWEB_HTOP_BUILD_APPS=ON \
  -DWEB_HTOP_BUILD_TESTS=ON

cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure --no-tests=error
```

Start the server and client in separate terminals:

```bash
./build/server/web_htop_server
./build/client/web_htop_client localhost 9999 8080
```

The server listens on 127.0.0.1 by default. HTTP uses port 8080; framed
telemetry uses port 9999.

## Record and replay

Capture every received generation without interrupting the live UI:

```bash
./build/client/web_htop_client localhost 9999 8080 \
  --record incident.jsonl
```

Replay it later without a server:

```bash
./build/client/web_htop_client --replay incident.jsonl
```

Request one machine-readable snapshot:

```bash
./build/client/web_htop_client localhost 9999 8080 --once
```

## HTTP API

```bash
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/ready
curl http://127.0.0.1:8080/metrics
curl http://127.0.0.1:8080/processes
curl http://127.0.0.1:8080/diagnostics
curl http://127.0.0.1:8080/exporter
```

| Endpoint | Contract |
|---|---|
| `/health` | Reactor liveness; independent of collector readiness |
| `/ready` | Required collectors are healthy and the snapshot is fresh |
| `/metrics` | Complete current version-2 generation |
| `/processes` | Top-K process set with sequence and truncation metadata |
| `/diagnostics` | Sessions, queue pressure, timeouts, encoding cost and snapshot age |
| `/exporter` | Low-cardinality Prometheus metrics about WEB HTOP itself |

HTTP is intentionally small: GET-only, bounded 8 KiB headers, one request per
connection, explicit deadlines, and no request bodies or keep-alive state machine.

## Streaming protocol

The stream is not a sequence of native C++ structures. Each message is a four-byte
big-endian length followed by an owned UTF-8 JSON document, capped at 8 MiB.

Every frame is a full snapshot and carries:

- protocol version;
- message type;
- server instance ID;
- monotonically increasing generation sequence;
- collection timestamps and section status.

After reconnect, a client needs only the next frame — there is no delta-recovery
protocol. A changed instance ID identifies a server restart; a sequence gap identifies
skipped complete generations.

## Built-in JSON Engine

WEB HTOP does not depend on a third-party JSON library. Its wire protocol and
HTTP responses are backed by a JSON parser and serializer implemented inside
the project.

The parser is not just a minimal tokenizer. It handles:

- objects, arrays, strings, booleans, null and typed numeric values;
- escaped characters and UTF-16 surrogate-pair decoding;
- signed, unsigned and floating-point number boundaries;
- stable ownership of decoded strings;
- configurable limits for input size, nesting depth, node count and object keys;
- rejection of duplicate keys, malformed escapes and invalid UTF-8;
- rejection of trailing data, non-finite values and incomplete documents;
- non-throwing parse failures through std::optional.

```cpp
#include "common/json/parser.hpp"

#include <iostream>
#include <string_view>

int main()
{
    constexpr std::string_view payload = R"({
        "protocol_version": 2,
        "sequence": 1842,
        "instance": "node-a",
        "ready": true,
        "load": [0.42, 0.31, 0.18]
    })";

    auto document = web_htop::json::Parse(payload);

    if (!document) {
        std::cerr << "invalid telemetry document\n";
        return 1;
    }

    const auto& root = document->value;

    const auto version = root["protocol_version"];
    const auto sequence = root["sequence"];
    const auto instance = root["instance"];

    std::cout
        << "protocol=" << version->get().AsUInt64().value()
        << " sequence=" << sequence->get().AsUInt64().value()
        << " instance=" << instance->get().AsString().value()
        << '\n';
}
```

Output:

```text
protocol=2 sequence=1842 instance=node-a
```

Malformed input is rejected instead of being partially accepted:

```cpp
auto trailing_comma = web_htop::json::Parse(R"({"ready":true,})");
auto broken_unicode = web_htop::json::Parse(R"({"name":"\uD800"})");
auto trailing_data   = web_htop::json::Parse(R"({"ready":true} garbage)");

assert(!trailing_comma);
assert(!broken_unicode);
assert(!trailing_data);
```

Keeping JSON inside the project makes protocol ownership explicit: parsing
limits, numeric semantics, string lifetime and validation rules remain aligned
with the telemetry model rather than being inherited from a general-purpose
dependency.

### Concurrency and Ownership Model

WEB HTOP strictly decouples metric sampling from client I/O using a single-writer, lock-free snapshot publication model:

- **Ownership Separation:** The telemetry sampler (`std::jthread`) owns `/proc` parsers, device state, and delta history. The network reactor (`epoll`) owns sockets, event notifications, and transmission queues.
- **Immutable Generations:** Each sampling tick serializes and frames a full `SystemSnapshot` into an immutable `PublishedSnapshot`. The reactor acquires this state via release-acquire semantics with zero mutex locks.
- **Latest-Wins Backpressure:** Connected sessions retain a bounded output buffer (one in-flight frame, one pending frame). Slow TCP observers never block the sampling worker or other connected dashboards.


## Tests and measurements

The test suite exercises more than successful parsing:

- every split point in framed TCP input;
- partial writes and several frames arriving together;
- stalled HTTP and TCP readers;
- counter resets, collector warm-up and PID reuse;
- descriptor reclamation and partial-startup rollback;
- shutdown while sessions have pending output;
- concurrent immutable publication;
- JSON ownership and parser limits.

GCC and Clang builds, ASan, UBSan, TSan and short fuzz runs are defined in CI.

Performance claims should come with the environment and raw data. The included
benchmark records request latency, collection time, server CPU, peak RSS, payload
size and reactor counters against synthetic procfs populations:

```bash
python3 scripts/benchmark.py \
  --server ./build/server/web_htop_server \
  --processes 5000 \
  --clients 10 \
  --requests 200 \
  --output benchmark-5000.json
```

## Project layout

```text
client/       terminal UI, replay and network client
server/       Linux collection, publication and epoll runtime
common/       owned JSON, models, protocol and RAII utilities
tests/        deterministic, integration and fuzz checks
tools/        optional scheduler profiler
packaging/    service and Kubernetes deployment files
scripts/      run, benchmark and validation helpers
docs/         contracts, architecture and operational notes
```
