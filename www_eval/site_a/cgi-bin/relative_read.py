#!/usr/bin/env python3

import json

result = {
    "ok": False,
    "value": "",
    "error": "",
}

try:
    with open("fixtures/relative_marker.txt", "r") as f:
        result["value"] = f.read().strip()
    result["ok"] = True
except Exception as exc:
    result["error"] = str(exc)

print("Content-Type: application/json")
print()
print(json.dumps(result, sort_keys=True))
