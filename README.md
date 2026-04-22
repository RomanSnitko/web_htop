# web_htop

Проект Web Htop представляет собой многослойную высокопроизводительную клиент-серверную систему мониторинга, реализующую сбор, агрегацию и потоковую доставку системных метрик в реальном времени. Серверная часть, построенная на низкоуровневом взаимодействии с Linux-подсистемой /proc, инкапсулирует набор специализированных collectors, формирующих консистентный snapshot состояния системы (CPU, память, сеть, диск, процессы) с временной синхронизацией и вычислением производных метрик. Центральный компонент SharedState обеспечивает потокобезопасный доступ к актуальным данным, выступая в роли in-memory data layer. Поверх него реализован транспортный уровень, включающий асинхронный HTTP API с маршрутизацией и обработчиками, а также streaming-механизм для непрерывной передачи телеметрии по TCP. Клиентская часть использует эти интерфейсы для визуализации и анализа, формируя интерактивное представление состояния системы. Архитектура проекта демонстрирует принципы модульности, изоляции ответственности и эффективной работы с системными ресурсами, приближаясь по сложности к production-grade observability решениям.

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
