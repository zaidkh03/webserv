#!/usr/bin/env python3
"""Fast smoke suite for eval-only workflow."""

import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[1]
SERVER_BIN = ROOT_DIR / "webserv"
CONFIG_PATH = ROOT_DIR / "config" / "eval_suite.conf"
LOG_PATH = ROOT_DIR / ".eval_smoke.log"

HOST = "127.0.0.1"
PORT_A = 18080
PORT_B = 18081

PASS = 0
FAIL = 0


def ok(name: str) -> None:
    global PASS
    PASS += 1
    print(f"PASS - {name}")


def bad(name: str, details: str) -> None:
    global FAIL
    FAIL += 1
    print(f"FAIL - {name}: {details}")


def run(cmd, timeout=8):
    return subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)


def curl(url, method=None, headers=None, data=None, head=False, timeout=8):
    cmd = ["curl", "-sS", "-D", "-", "-o", "-"]
    if head:
        cmd.append("-I")
    if method:
        cmd += ["-X", method]
    if headers:
        for k, v in headers.items():
            cmd += ["-H", f"{k}: {v}"]
    if data is not None:
        cmd += ["--data", data]
    cmd += ["--connect-timeout", "2", "--max-time", str(timeout), url]
    proc = run(cmd, timeout=timeout + 2)
    if proc.returncode != 0:
        return None, None, f"curl rc={proc.returncode} err={proc.stderr.strip()}"

    raw = proc.stdout
    sep = "\r\n\r\n"
    if sep not in raw:
        sep = "\n\n"
    parts = raw.split(sep, 1)
    if len(parts) != 2:
        return None, None, "invalid HTTP split"
    headers_raw, body = parts
    first = headers_raw.splitlines()[0] if headers_raw else ""
    tokens = first.split()
    if len(tokens) < 2:
        return None, None, f"bad status line: {first}"
    try:
        code = int(tokens[1])
    except Exception:
        return None, None, f"non-int status: {first}"
    return code, {"raw": headers_raw}, body


def wait_http(port, tries=40):
    for _ in range(tries):
        code, _, _ = curl(f"http://{HOST}:{port}/", timeout=2)
        if code is not None:
            return True
        time.sleep(0.2)
    return False


def main() -> int:
    if not CONFIG_PATH.exists():
        print(f"Missing config: {CONFIG_PATH}")
        return 1

    if not SERVER_BIN.exists():
        build = run(["make", "-C", str(ROOT_DIR), "-j4"], timeout=120)
        if build.returncode != 0:
            print(build.stdout)
            print(build.stderr)
            print("Build failed")
            return 1

    with open(LOG_PATH, "w") as logf:
        proc = subprocess.Popen(
            [str(SERVER_BIN), str(CONFIG_PATH)],
            stdout=logf,
            stderr=logf,
            text=True,
        )

    try:
        if not wait_http(PORT_A) or not wait_http(PORT_B):
            bad("server startup", f"not ready on {PORT_A}/{PORT_B}")
            return 1
        ok("server startup")

        code, _, body = curl(f"http://{HOST}:{PORT_A}/")
        if code == 200 and body and "EVAL SITE A ROOT" in body:
            ok("GET site A root")
        else:
            bad("GET site A root", f"code={code}")

        code, _, body = curl(f"http://{HOST}:{PORT_B}/")
        if code == 200 and body and "EVAL SITE B ROOT" in body:
            ok("GET site B root")
        else:
            bad("GET site B root", f"code={code}")

        code, hdr, _ = curl(f"http://{HOST}:{PORT_A}/redirect-me")
        if code == 301 and hdr and "Location: /with-index/" in hdr["raw"]:
            ok("redirect route")
        else:
            bad("redirect route", f"code={code}")

        payload = "smoke_payload"
        code, _, _ = curl(
            f"http://{HOST}:{PORT_A}/upload/smoke.txt",
            method="POST",
            headers={"Content-Type": "text/plain"},
            data=payload,
        )
        if code == 201:
            ok("upload file")
        else:
            bad("upload file", f"code={code}")

        code, _, body = curl(f"http://{HOST}:{PORT_A}/upload/smoke.txt")
        if code == 200 and body == payload:
            ok("read uploaded file")
        else:
            bad("read uploaded file", f"code={code}")

        code, _, _ = curl(f"http://{HOST}:{PORT_A}/delete/smoke.txt", method="DELETE")
        if code == 204:
            ok("delete uploaded file")
        else:
            bad("delete uploaded file", f"code={code}")

        code, _, body = curl(f"http://{HOST}:{PORT_A}/cgi-bin/env_echo.py?alpha=1")
        if code == 200:
            try:
                data = json.loads(body)
                if data.get("method") == "GET" and data.get("query_string") == "alpha=1":
                    ok("cgi env echo")
                else:
                    bad("cgi env echo", "unexpected JSON fields")
            except Exception as exc:
                bad("cgi env echo", f"json parse error: {exc}")
        else:
            bad("cgi env echo", f"code={code}")

        code, hdr, _ = curl(f"http://{HOST}:{PORT_A}/post-only", head=True)
        if code == 405 and hdr and "Content-Length:" in hdr["raw"]:
            ok("HEAD policy")
        else:
            bad("HEAD policy", f"code={code}")

    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=5)

    print(f"\nSummary: PASS={PASS} FAIL={FAIL}")
    if FAIL == 0:
        print("Smoke suite passed")
        return 0
    print("Smoke suite failed")
    return 1


if __name__ == "__main__":
    sys.exit(main())
