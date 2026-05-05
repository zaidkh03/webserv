#!/usr/bin/env python3

import json
import os
import sys


def safe_int(value):
    try:
        return int(value)
    except Exception:
        return 0


method = os.environ.get("REQUEST_METHOD", "")
query_string = os.environ.get("QUERY_STRING", "")
request_uri = os.environ.get("REQUEST_URI", "")
script_name = os.environ.get("SCRIPT_NAME", "")
path_info = os.environ.get("PATH_INFO", "")
server_protocol = os.environ.get("SERVER_PROTOCOL", "")
content_length_raw = os.environ.get("CONTENT_LENGTH", "")
content_length = safe_int(content_length_raw)

body = ""
if method in ("POST", "PUT"):
    if content_length > 0:
        body = os.read(0, content_length).decode("utf-8", "replace")
    else:
        body = sys.stdin.read()

payload = {
    "method": method,
    "query_string": query_string,
    "request_uri": request_uri,
    "script_name": script_name,
    "path_info": path_info,
    "server_protocol": server_protocol,
    "content_length": content_length,
    "body_size": len(body),
    "body": body,
}

print("Content-Type: application/json")
print()
print(json.dumps(payload, sort_keys=True))
