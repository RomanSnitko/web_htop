# web_htop

`web_htop` is a C++17 monitoring project with server/client architecture.

- Server collects Linux metrics and process list.
- Server exposes HTTP API (`/health`, `/metrics`, `/processes`).
- Server pushes live snapshots over streaming TCP.
- Client connects to streaming server and renders terminal dashboard + process table.

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
