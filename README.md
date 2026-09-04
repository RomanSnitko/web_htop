# web_htop

The server collects system state directly from the Linux /proc filesystem through dedicated collectors responsible for CPU, memory, network, disk, and process metrics. Collectors produce synchronized system snapshots and calculate derived metrics where required. The collected state is stored in a shared in-memory data layer that provides thread-safe access to the latest snapshot for concurrent consumers.

The transport layer exposes two interfaces: an asynchronous HTTP API for querying health, system metrics, and process information, and a TCP streaming channel for continuous delivery of live snapshots. The client consumes the streaming data and renders it as a terminal dashboard with system and process views.

The project separates metric collection, state management, transport, and presentation into independent components, allowing the data acquisition layer to operate independently from network clients and presentation logic. The implementation focuses on Linux system interfaces, concurrent access to shared state, asynchronous I/O, and continuous telemetry delivery.

- Server collects Linux metrics and process list.
- Server exposes HTTP API (`/health`, `/metrics`, `/processes`).
- Server pushes live snapshots over streaming TCP.
- Client connects to streaming server and renders terminal dashboard + process table.

<img width="724" height="541" alt="image" src="https://github.com/user-attachments/assets/86f82031-c65d-4d8f-bb8f-9c6b28b9b035" />

## Build

```bash
cmake -S . -B build -DWEB_HTOP_BUILD_APPS=ON -DWEB_HTOP_BUILD_TESTS=ON
cmake --build build -j4
```

## Run

```bash
./build/server/web_htop_server
./build/client/web_htop_client localhost 9999 8080
```

Helper scripts:

```bash
bash scripts/run_server.sh
bash scripts/run_client.sh localhost 9999 8080
```

## Verify HTTP API

```bash
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/metrics
curl http://127.0.0.1:8080/processes
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

## Documentation

- [Build and run guide](docs/build_run.md)
- [Architecture notes](docs/architecture.md)
- [Branch/module plan](docs/description_branches.md)
