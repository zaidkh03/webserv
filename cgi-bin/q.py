#!/usr/bin/python3
import os

query_string = os.environ.get('QUERY_STRING', '')
script_name = os.environ.get('SCRIPT_NAME', '')
method = os.environ.get('REQUEST_METHOD', '')

print("Content-Type: text/html\r")
print("\r")

print("<html><body>")
print(f"<p>string={query_string}</p>")
print(f"<p>name={script_name}</p>")
print(f"<p>method={method.lower()}</p>")
print("</body></html>")
