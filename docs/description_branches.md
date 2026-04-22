# feature/project-setup


настройка CMakeLists.txt
подключение библиотек
структура папок
базовые цели сборки
общие compile options
.gitignore
возможно README.md
логгеры
мелкие утилиты разработки
скрипты запуска
форматирование


# feature/common-models


структуры для:
CPU
RAM
Disk
Network
ProcessInfo
SystemSnapshot
enums
protocol message types
сериализация
json


# docs/project-docs


architecture.md
build_run.md
http_api.md
live_protocol.md
data_models.md
module_responsibilities.md
прочие markdown-файлы


# feature/server-core

Это каркас серверного приложения.

Что туда входит
main.cpp сервера
ServerApp
ConfigManager
SharedState
запуск/остановка сервера
базовая инициализация
обработка сигналов завершения
Чего там не должно быть
сбор метрик
процессы
TCP transport
HTTP handlers


# feature/server-collectors

Это ветка для обычных системных метрик.

Что туда входит
CPU collector
memory collector
disk collector
network collector
возможно loadavg collector
MetricsCollector
чтение /proc/stat, /proc/meminfo, /proc/net/dev
statvfs()
Почему она отдельная

Потому что это один логический блок: сбор метрик системы.


# feature/server-processes

Вот здесь как раз отвечаю на твой вопрос “я не понял зачем”.

Зачем нужна отдельная ветка процессов

Потому что процессы — это не просто ещё одна метрика, а отдельный сложный функциональный блок.

Системные метрики типа CPU/RAM/disk/network собираются сравнительно просто:

читаем один-два файла;
считаем значения;
всё.

А процессы — это уже:

обход /proc;
поиск PID-директорий;
чтение /proc/[pid]/comm;
чтение /proc/[pid]/stat;
чтение /proc/[pid]/status;
вычисление CPU% по процессу;
хранение истории по PID;
сортировка top N;
обработка случая, когда процесс исчез в момент чтения.
То есть эта ветка нужна для
списка процессов;
процессных метрик;
process history;
process collector.


# feature/server-http-api

Это отдельная ветка под HTTP.

Что туда входит
http_server
http_handlers
эндпоинты:
GET /health
GET /metrics
GET /processes
POST /config/update-interval
Почему отдельно от streaming

Потому что:

live transport и HTTP — разные каналы;
их удобнее делать и тестировать независимо.


# feature/client-core

Это каркас клиента.

Что туда входит
main.cpp
ClientApp
ClientState
запуск/остановка клиента
хранение последнего snapshot
хранение статуса соединения


# feature/client-network

Это ветка live-подключения клиента.

Что туда входит
подключение к TCP server
чтение стрима
reconnect
stream reader
обновление ClientState
Почему отдельно от client-core

Потому что client-core — это каркас, а client-network — конкретная сетевая логика.


# feature/client-http-control

Это HTTP-клиентская часть.

Что туда входит
вызовы к HTTP API сервера
запросы:
получить /metrics
получить /processes
поменять интервал обновления
возможно future actions
Почему отдельно

Потому что live stream и HTTP — две разные модели взаимодействия.


# feature/client-terminal-ui

Это общий терминальный интерфейс.

Что туда входит
TerminalUi
dashboard
status bar
help line
форматирование значений
цветовая индикация
Что не входит
таблица процессов, если вы хотите держать её отдельно


# feature/client-process-table

Это UI и логика списка процессов.

Что туда входит
таблица процессов
сортировка
фильтрация
форматирование колонок
Почему отдельно

Потому что список процессов может оказаться самым тяжёлым куском UI:

много строк;
сортировки;
фильтры;
отдельная логика отображения.

Если хотите упростить, можно потом слить это в client-terminal-ui, но как отдельная ветка это нормально.


# test/core-tests

Это ветка тестов.

Что туда входит
тесты сериализации
тесты протокола
тесты collectors
тесты process collector
тесты сортировки/фильтрации
Почему не feature

Потому что это отдельный тип работы.