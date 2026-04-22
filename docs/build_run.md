# Build and Run

## 1. Configure and build

```bash
cd web_htop
cmake -S . -B build -DWEB_HTOP_BUILD_APPS=ON -DWEB_HTOP_BUILD_TESTS=ON
cmake --build build -j4
```

## 2. Run server

```bash
./build/server/web_htop_server
```

Server defaults:
- HTTP API: `0.0.0.0:8080`
- Streaming TCP: `0.0.0.0:9999`

## 3. Run client

```bash
./build/client/web_htop_client localhost 9999 8080
```

Or with helper script:

```bash
bash scripts/run_client.sh localhost 9999 8080
```

## 4. Verify HTTP API

```bash
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/metrics
curl -i http://127.0.0.1:8080/processes
```

## 5. Verify streaming protocol

Each message is a 4-byte big-endian payload length + JSON payload (`SystemSnapshot`).

Quick check via Python:

```bash
python3 - <<'PY'
import socket, struct
s = socket.create_connection(("127.0.0.1", 9999), timeout=3)
raw_len = s.recv(4)
size = struct.unpack("!I", raw_len)[0]
payload = b""
while len(payload) < size:
    payload += s.recv(size - len(payload))
print(payload.decode("utf-8")[:200] + "...")
s.close()
PY
```

## 6. Run tests

```bash
ctest --test-dir build --output-on-failure
```