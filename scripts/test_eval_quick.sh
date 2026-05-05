#!/usr/bin/env bash

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"

HOST="${HOST:-127.0.0.1}"
PORT_A="${PORT_A:-18080}"
PORT_B="${PORT_B:-18081}"
PORT_SECONDARY="${PORT_SECONDARY:-18082}"

SERVER_BIN="${SERVER_BIN:-$ROOT_DIR/webserv}"
CONFIG_PATH="${CONFIG_PATH:-$ROOT_DIR/config/eval_suite.conf}"
SECONDARY_CONFIG_PATH="${SECONDARY_CONFIG_PATH:-$ROOT_DIR/config/eval_secondary.conf}"
CONFLICT_CONFIG_PATH="${CONFLICT_CONFIG_PATH:-$ROOT_DIR/config/eval_port_conflict.conf}"

AUTO_BUILD="${AUTO_BUILD:-1}"
CONNECT_TIMEOUT="${CONNECT_TIMEOUT:-2}"
MAX_TIME="${MAX_TIME:-8}"
IDLE_WAIT_SECONDS="${IDLE_WAIT_SECONDS:-34}"

TMP_DIR="${TMP_DIR:-$ROOT_DIR/.eval_quick_tmp}"
LOG_MAIN="$TMP_DIR/main_server.log"
LOG_SECONDARY="$TMP_DIR/secondary_server.log"
LOG_CONFLICT="$TMP_DIR/conflict_server.log"
BODY_FILE="$TMP_DIR/response.body"
HEADERS_FILE="$TMP_DIR/response.headers"

MAIN_PID=""
SECONDARY_PID=""
LAST_CODE="000"

PASS_COUNT=0
FAIL_COUNT=0
TEST_COUNT=0

GREEN="\033[0;32m"
RED="\033[0;31m"
CYAN="\033[0;36m"
YELLOW="\033[1;33m"
NC="\033[0m"

info() { echo -e "${CYAN}$1${NC}"; }
warn() { echo -e "${YELLOW}$1${NC}"; }

pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${GREEN}PASS${NC} - $1"
}

fail() {
    FAIL_COUNT=$((FAIL_COUNT + 1))
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${RED}FAIL${NC} - $1"
}

cleanup() {
    if [ -n "$SECONDARY_PID" ] && kill -0 "$SECONDARY_PID" 2>/dev/null; then
        kill "$SECONDARY_PID" >/dev/null 2>&1 || true
        wait "$SECONDARY_PID" 2>/dev/null || true
    fi
    if [ -n "$MAIN_PID" ] && kill -0 "$MAIN_PID" 2>/dev/null; then
        kill "$MAIN_PID" >/dev/null 2>&1 || true
        wait "$MAIN_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT

reset_upload_dir() {
    find "$ROOT_DIR/www_eval/site_a/uploads" -mindepth 1 -type f ! -name '.gitkeep' -delete 2>/dev/null || true
}

run_cmd_test() {
    local label="$1"
    shift

    if "$@"; then
        pass "$label"
    else
        fail "$label"
    fi
}

port_is_reachable() {
    local host="$1"
    local port="$2"
    python3 - "$host" "$port" <<'PY'
import socket
import sys

host = sys.argv[1]
port = int(sys.argv[2])
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(0.5)
rc = s.connect_ex((host, port))
s.close()
sys.exit(0 if rc == 0 else 1)
PY
}

require_port_free() {
    local port="$1"
    if port_is_reachable "$HOST" "$port"; then
        warn "Port $port is already in use on $HOST. Stop existing webserv processes first."
        return 1
    fi
    return 0
}

wait_for_http() {
    local port="$1"
    local i
    for i in $(seq 1 40); do
        LAST_CODE=$(curl -s -o /dev/null -w "%{http_code}" \
            --connect-timeout "$CONNECT_TIMEOUT" \
            --max-time "$MAX_TIME" \
            "http://$HOST:$port/")
        if [ "$LAST_CODE" != "000" ]; then
            return 0
        fi
        sleep 0.2
    done
    return 1
}

start_main_server() {
    require_port_free "$PORT_A" || return 1
    require_port_free "$PORT_B" || return 1

    mkdir -p "$TMP_DIR"
    : > "$LOG_MAIN"
    "$SERVER_BIN" "$CONFIG_PATH" > "$LOG_MAIN" 2>&1 &
    MAIN_PID=$!
    if ! wait_for_http "$PORT_A"; then
        warn "Main server failed to become ready."
        tail -n 50 "$LOG_MAIN" || true
        return 1
    fi
    if ! kill -0 "$MAIN_PID" 2>/dev/null; then
        warn "Main server process exited during startup."
        tail -n 50 "$LOG_MAIN" || true
        return 1
    fi
    return 0
}

start_secondary_server() {
    require_port_free "$PORT_SECONDARY" || return 1

    : > "$LOG_SECONDARY"
    "$SERVER_BIN" "$SECONDARY_CONFIG_PATH" > "$LOG_SECONDARY" 2>&1 &
    SECONDARY_PID=$!
    if ! wait_for_http "$PORT_SECONDARY"; then
        warn "Secondary server failed to become ready."
        tail -n 50 "$LOG_SECONDARY" || true
        return 1
    fi
    if ! kill -0 "$SECONDARY_PID" 2>/dev/null; then
        warn "Secondary server process exited during startup."
        tail -n 50 "$LOG_SECONDARY" || true
        return 1
    fi
    return 0
}

curl_capture() {
    LAST_CODE=$(curl -sS -D "$HEADERS_FILE" -o "$BODY_FILE" -w "%{http_code}" \
        --connect-timeout "$CONNECT_TIMEOUT" --max-time "$MAX_TIME" "$@")
    return $?
}

assert_status() {
    local label="$1"
    local expected="$2"
    shift 2

    if ! curl_capture "$@"; then
        fail "$label (curl failure)"
        return
    fi

    if [ "$LAST_CODE" = "$expected" ]; then
        pass "$label (status $LAST_CODE)"
    else
        fail "$label (status $LAST_CODE expected $expected)"
    fi
}

assert_body_contains() {
    local label="$1"
    local needle="$2"
    if grep -Fq "$needle" "$BODY_FILE"; then
        pass "$label"
    else
        fail "$label"
    fi
}

assert_body_not_contains() {
    local label="$1"
    local needle="$2"
    if grep -Fq "$needle" "$BODY_FILE"; then
        fail "$label"
    else
        pass "$label"
    fi
}

assert_header_contains() {
    local label="$1"
    local needle="$2"
    if grep -Fiq "$needle" "$HEADERS_FILE"; then
        pass "$label"
    else
        fail "$label"
    fi
}

run_python_test() {
    local label="$1"
    shift

    local out_file="$TMP_DIR/py_test.out"
    : > "$out_file"
    if "$@" > "$out_file" 2>&1; then
        pass "$label"
    else
        fail "$label"
        sed -n '1,120p' "$out_file" | sed 's/^/  /'
    fi
}

check_server_alive() {
    curl -s -o /dev/null --connect-timeout "$CONNECT_TIMEOUT" --max-time "$MAX_TIME" "http://$HOST:$PORT_A/with-index/"
}

test_basic_and_routes() {
    info "Running basic mandatory checks"

    assert_status "GET / on site A" "200" "http://$HOST:$PORT_A/"
    assert_body_contains "Site A marker" "EVAL SITE A ROOT"

    assert_status "GET / on site B" "200" "http://$HOST:$PORT_B/"
    assert_body_contains "Site B marker" "EVAL SITE B ROOT"

    assert_status "GET /with-index" "200" "http://$HOST:$PORT_A/with-index"
    assert_body_contains "with-index marker" "EVAL WITH INDEX PAGE"

    assert_status "GET /with-index/" "200" "http://$HOST:$PORT_A/with-index/"

    assert_status "GET redirect route" "301" "http://$HOST:$PORT_A/redirect-me"
    assert_header_contains "Redirect has Location header" "Location: /with-index/"

    assert_status "Custom 404 page" "404" "http://$HOST:$PORT_A/this-page-does-not-exist"
    assert_body_contains "404 custom body marker" "EVAL CUSTOM 404 PAGE"

    assert_status "DELETE denied on /" "405" -X DELETE "http://$HOST:$PORT_A/"
}

test_upload_delete_and_limits() {
    info "Running upload/delete/body limit checks"

    local payload="mandatory-upload-payload-42"

    assert_status "Upload file via POST" "201" -X POST -H "Content-Type: text/plain" \
        --data "$payload" "http://$HOST:$PORT_A/upload/eval_upload.txt"

    assert_status "Retrieve uploaded file" "200" "http://$HOST:$PORT_A/upload/eval_upload.txt"
    assert_body_contains "Uploaded payload must match" "$payload"

    assert_status "DELETE uploaded file" "204" -X DELETE "http://$HOST:$PORT_A/delete/eval_upload.txt"

    assert_status "Uploaded file gone after DELETE" "404" "http://$HOST:$PORT_A/upload/eval_upload.txt"

    local big
    big=$(python3 - <<'PY'
print("A" * 64)
PY
)
    assert_status "POST over tiny-body max limit" "413" -X POST -H "Content-Type: text/plain" \
        --data "$big" "http://$HOST:$PORT_A/tiny-body/too_big.txt"
}

test_autoindex_and_dir_edges() {
    info "Running autoindex and directory edge-case checks"

    assert_status "GET /list/" "200" "http://$HOST:$PORT_A/list/"
    assert_body_contains "Autoindex HTML format" "<html"
    assert_body_contains "Autoindex contains expected fixture A" "autoindex_expected_a.txt"
    assert_body_contains "Autoindex contains expected fixture B" "autoindex_expected_b.txt"
    assert_body_not_contains "Autoindex excludes outside marker" "outside_marker.txt"

    assert_status "GET /list (no trailing slash)" "200" "http://$HOST:$PORT_A/list"
    assert_body_contains "No-slash listing still contains fixture" "autoindex_expected_a.txt"
}

test_head_policy() {
    info "Running HEAD policy checks"

    assert_status "HEAD allowed on GET route" "200" -I "http://$HOST:$PORT_A/with-index/"
    assert_header_contains "HEAD response has Content-Length" "Content-Length:"

    assert_status "HEAD denied on POST-only route" "405" -I "http://$HOST:$PORT_A/post-only"
}

test_cgi_env_and_behavior() {
    info "Running CGI checks (env + strict behavior)"

    assert_status "CGI GET env echo" "200" "http://$HOST:$PORT_A/cgi-bin/env_echo.py?alpha=1&beta=two"
    run_python_test "CGI GET env values" python3 - "$BODY_FILE" <<'PY'
import json
import sys

path = sys.argv[1]
with open(path, 'r') as f:
    data = json.load(f)

assert data.get("method") == "GET", data
assert data.get("query_string") == "alpha=1&beta=two", data
assert data.get("request_uri") == "/cgi-bin/env_echo.py?alpha=1&beta=two", data
assert data.get("script_name") == "/cgi-bin/env_echo.py", data
PY

    local post_payload="known_payload_123"
    assert_status "CGI POST env echo" "200" -X POST -H "Content-Type: text/plain" \
        --data "$post_payload" "http://$HOST:$PORT_A/cgi-bin/env_echo.py"

    run_python_test "CGI POST content-length/body-size/body" python3 - "$BODY_FILE" "$post_payload" <<'PY'
import json
import sys

path = sys.argv[1]
payload = sys.argv[2]
with open(path, 'r') as f:
    data = json.load(f)

assert data.get("method") == "POST", data
assert data.get("content_length") == len(payload), data
assert data.get("body_size") == len(payload), data
assert data.get("body") == payload, data
PY

    if curl_capture "http://$HOST:$PORT_A/cgi-bin/env_echo.py/extra?x=1" && [ "$LAST_CODE" = "200" ]; then
        run_python_test "CGI PATH_INFO strict check" python3 - "$BODY_FILE" <<'PY'
import json
import sys

with open(sys.argv[1], 'r') as f:
    data = json.load(f)

assert data.get("path_info") == "/extra", data
PY
    else
        fail "CGI PATH_INFO strict check (expected 200 with PATH_INFO, got $LAST_CODE)"
    fi

    assert_status "CGI relative file access route" "200" "http://$HOST:$PORT_A/cgi-bin/relative_read.py"
    run_python_test "CGI relative file access must succeed" python3 - "$BODY_FILE" <<'PY'
import json
import sys

with open(sys.argv[1], 'r') as f:
    data = json.load(f)

assert data.get("ok") is True, data
assert data.get("value") == "RELATIVE_FIXTURE_OK", data
PY

    if curl_capture "http://$HOST:$PORT_A/cgi-bin/crash.py"; then
        if [ "$LAST_CODE" = "200" ]; then
            fail "CGI crash script must not return 200"
        else
            pass "CGI crash script returns non-200 ($LAST_CODE)"
        fi
    else
        pass "CGI crash script connection failed (acceptable non-200 behavior)"
    fi

    if check_server_alive; then
        pass "Server alive after CGI crash case"
    else
        fail "Server alive after CGI crash case"
    fi

    assert_status "CGI short sleep still responds" "200" "http://$HOST:$PORT_A/cgi-bin/sleep_short.py"
}

test_malformed_http() {
    info "Running malformed HTTP checks"

    run_python_test "Malformed request: unknown method line" python3 - "$HOST" "$PORT_A" <<'PY'
import socket
import sys

host = sys.argv[1]
port = int(sys.argv[2])
req = b"GETTTTT / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(3)
s.connect((host, port))
s.sendall(req)
resp = s.recv(4096).decode("latin1", "ignore")
s.close()
line = resp.split("\r\n", 1)[0]
parts = line.split()
assert len(parts) >= 2, line
code = int(parts[1])
assert 400 <= code <= 599, line
PY

    if check_server_alive; then
        pass "Server alive after unknown-method malformed request"
    else
        fail "Server alive after unknown-method malformed request"
    fi

    run_python_test "Malformed request: broken header" python3 - "$HOST" "$PORT_A" <<'PY'
import socket
import sys

host = sys.argv[1]
port = int(sys.argv[2])
req = b"GET / HTTP/1.1\r\nHost 127.0.0.1\r\n\r\n"

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(3)
s.connect((host, port))
s.sendall(req)
resp = s.recv(4096).decode("latin1", "ignore")
s.close()
line = resp.split("\r\n", 1)[0]
parts = line.split()
assert len(parts) >= 2, line
code = int(parts[1])
assert 400 <= code <= 599, line
PY

    if check_server_alive; then
        pass "Server alive after broken-header malformed request"
    else
        fail "Server alive after broken-header malformed request"
    fi
}

test_non_blocking_core() {
    info "Running non-blocking core tests"

    run_python_test "Slow-client does not block fast request" python3 - "$HOST" "$PORT_A" <<'PY'
import socket
import sys
import time

host = sys.argv[1]
port = int(sys.argv[2])

sa = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sa.settimeout(5)
sa.connect((host, port))

slow_prefix = b"GET /with-index/ HTTP/1.1\r\nHost: 127.0.0.1\r\nUser-Agent: slow"
for b in slow_prefix:
    sa.send(bytes([b]))
    time.sleep(0.02)

start = time.time()
sb = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sb.settimeout(3)
sb.connect((host, port))
sb.sendall(b"GET /with-index/ HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
resp = sb.recv(4096).decode("latin1", "ignore")
sb.close()
elapsed = time.time() - start
line = resp.split("\r\n", 1)[0]
assert " 200 " in line, line
assert elapsed < 1.5, elapsed

sa.sendall(b"\r\nConnection: close\r\n\r\n")
resp2 = sa.recv(4096).decode("latin1", "ignore")
sa.close()
assert " 200 " in resp2.split("\r\n", 1)[0], resp2[:100]
PY

    run_python_test "Partial POST body does not block other clients" python3 - "$HOST" "$PORT_A" <<'PY'
import socket
import sys
import time

host = sys.argv[1]
port = int(sys.argv[2])
body = b"partial-post-body-ok"

sa = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sa.settimeout(8)
sa.connect((host, port))

headers = (
    b"POST /upload/partial_slow.txt HTTP/1.1\r\n"
    b"Host: 127.0.0.1\r\n"
    b"Content-Type: text/plain\r\n"
    b"Content-Length: " + str(len(body)).encode() + b"\r\n"
    b"Connection: close\r\n\r\n"
)
sa.sendall(headers)

chunks = [body[:5], body[5:10], body[10:15], body[15:]]
sa.sendall(chunks[0])
time.sleep(0.2)

start = time.time()
sb = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sb.settimeout(3)
sb.connect((host, port))
sb.sendall(b"GET /with-index/ HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
resp = sb.recv(4096).decode("latin1", "ignore")
sb.close()
elapsed = time.time() - start
assert " 200 " in resp.split("\r\n", 1)[0], resp[:120]
assert elapsed < 1.5, elapsed

for c in chunks[1:]:
    time.sleep(0.2)
    sa.sendall(c)

resp2 = sa.recv(4096).decode("latin1", "ignore")
sa.close()
assert " 201 " in resp2.split("\r\n", 1)[0], resp2[:120]
PY

    run_python_test "Chunked arrival request is handled" python3 - "$HOST" "$PORT_A" <<'PY'
import json
import socket
import sys
import time

host = sys.argv[1]
port = int(sys.argv[2])

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(5)
s.connect((host, port))

headers = (
    b"POST /cgi-bin/env_echo.py HTTP/1.1\r\n"
    b"Host: 127.0.0.1\r\n"
    b"Transfer-Encoding: chunked\r\n"
    b"Content-Type: text/plain\r\n"
    b"Connection: close\r\n\r\n"
)
s.sendall(headers)

parts = [b"5\r\nhello\r\n", b"6\r\n world\r\n", b"0\r\n\r\n"]
for part in parts:
    s.sendall(part)
    time.sleep(0.15)

buf = b""
while True:
    try:
        chunk = s.recv(4096)
    except socket.timeout:
        break
    if not chunk:
        break
    buf += chunk
s.close()

raw = buf.decode("utf-8", "ignore")
line = raw.split("\r\n", 1)[0]
assert " 200 " in line, line

if "\r\n\r\n" in raw:
    body = raw.split("\r\n\r\n", 1)[1]
else:
    body = raw.split("\n\n", 1)[1]

j = json.loads(body)
assert j.get("body") == "hello world", j
assert j.get("body_size") == 11, j
PY

    run_python_test "Concurrent sockets high success ratio" python3 - "$HOST" "$PORT_A" <<'PY'
import socket
import sys
import threading

host = sys.argv[1]
port = int(sys.argv[2])
N = 40
ok = 0
lock = threading.Lock()


def worker():
    global ok
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3)
        s.connect((host, port))
        s.sendall(b"GET /with-index/ HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
        data = s.recv(4096).decode("latin1", "ignore")
        s.close()
        line = data.split("\r\n", 1)[0]
        if " 200 " in line:
            with lock:
                ok += 1
    except Exception:
        pass

threads = [threading.Thread(target=worker) for _ in range(N)]
for t in threads:
    t.start()
for t in threads:
    t.join()

ratio = float(ok) / float(N)
assert ratio >= 0.9, (ok, N, ratio)
PY
}

test_idle_timeout_and_hang() {
    info "Running idle timeout / hanging-request checks"

    run_python_test "Idle connection does not block and gets timed out" python3 - "$HOST" "$PORT_A" "$IDLE_WAIT_SECONDS" <<'PY'
import socket
import sys
import time

host = sys.argv[1]
port = int(sys.argv[2])
idle_wait = int(sys.argv[3])

idle = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
idle.settimeout(3)
idle.connect((host, port))

# While idle socket is open, ensure regular requests still complete.
for _ in range(3):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3)
    s.connect((host, port))
    s.sendall(b"GET /with-index/ HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    resp = s.recv(4096).decode("latin1", "ignore")
    s.close()
    assert " 200 " in resp.split("\r\n", 1)[0], resp[:100]

# Wait past server idle timeout.
time.sleep(idle_wait)

try:
    idle.sendall(b"GET /with-index/ HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    data = idle.recv(4096)
    # Expected closed connection after timeout.
    assert data == b"", data[:80]
except (BrokenPipeError, ConnectionResetError):
    pass
finally:
    idle.close()
PY
}

test_port_behavior() {
    info "Running port behavior checks"

    timeout 5 "$SERVER_BIN" "$CONFLICT_CONFIG_PATH" > "$LOG_CONFLICT" 2>&1
    local conflict_rc=$?

    if [ "$conflict_rc" -eq 124 ]; then
        fail "Conflict config must fail fast (it timed out)"
    elif [ "$conflict_rc" -eq 0 ]; then
        fail "Conflict config must not exit successfully"
    else
        pass "Conflict config fails as expected (rc=$conflict_rc)"
    fi

    if start_secondary_server; then
        pass "Secondary webserv instance started on different port"
    else
        fail "Secondary webserv instance started on different port"
        return
    fi

    assert_status "Primary instance still serves on port A" "200" "http://$HOST:$PORT_A/"
    assert_body_contains "Primary marker after secondary start" "EVAL SITE A ROOT"

    assert_status "Secondary instance serves on dedicated port" "200" "http://$HOST:$PORT_SECONDARY/"
    assert_body_contains "Secondary marker content" "EVAL SITE B ROOT"

    if [ -n "$SECONDARY_PID" ] && kill -0 "$SECONDARY_PID" 2>/dev/null; then
        kill "$SECONDARY_PID" >/dev/null 2>&1 || true
        wait "$SECONDARY_PID" 2>/dev/null || true
        SECONDARY_PID=""
    fi
}

print_summary() {
    echo
    echo "========================================"
    echo "Eval Quick Suite Summary"
    echo "Total:  $TEST_COUNT"
    echo -e "Pass:   ${GREEN}$PASS_COUNT${NC}"
    echo -e "Fail:   ${RED}$FAIL_COUNT${NC}"
    echo "========================================"

    if [ "$FAIL_COUNT" -eq 0 ]; then
        echo "RESULT: PASS"
        return 0
    fi

    echo "RESULT: FAIL"
    return 1
}

main() {
    mkdir -p "$TMP_DIR"

    if [ "$AUTO_BUILD" -eq 1 ]; then
        info "Building project"
        if ! make -C "$ROOT_DIR" -j4 >/dev/null; then
            fail "Build failed"
            print_summary
            exit 1
        fi
        pass "Build succeeded"
    fi

    if [ ! -x "$SERVER_BIN" ]; then
        fail "Server binary not executable: $SERVER_BIN"
        print_summary
        exit 1
    fi

    info "Starting main server using $CONFIG_PATH"
    if ! start_main_server; then
        fail "Main server started"
        print_summary
        exit 1
    fi
    pass "Main server started"
    reset_upload_dir

    test_basic_and_routes
    test_upload_delete_and_limits
    test_autoindex_and_dir_edges
    test_head_policy
    test_cgi_env_and_behavior
    test_malformed_http
    test_non_blocking_core
    test_idle_timeout_and_hang
    test_port_behavior

    reset_upload_dir

    print_summary
}

main "$@"
