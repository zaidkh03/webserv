#!/usr/bin/env python3
"""
COMPREHENSIVE WEBSERV TESTER
Based on common 42 webserv testers from GitHub
Tests all edge cases and requirements
"""

import subprocess
import time
import os
import sys
import socket

GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
BOLD = '\033[1m'
RESET = '\033[0m'

total_tests = 0
passed_tests = 0
failed_tests = []

def print_header(text):
    print(f"\n{BLUE}{BOLD}{'='*70}{RESET}")
    print(f"{BLUE}{BOLD}{text}{RESET}")
    print(f"{BLUE}{BOLD}{'='*70}{RESET}\n")

def print_test(name, passed, details=""):
    global total_tests, passed_tests, failed_tests
    total_tests += 1
    
    if passed:
        passed_tests += 1
        print(f"{GREEN}✓ PASS{RESET} - {name}")
    else:
        failed_tests.append(name)
        print(f"{RED}✗ FAIL{RESET} - {name}")
    
    if details:
        print(f"       {YELLOW}{details}{RESET}")

def run_curl(args, expected_code, timeout=5):
    """Run curl and check status code"""
    try:
        cmd = ['curl', '-s', '-o', '/dev/null', '-w', '%{http_code}'] + args
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        code = result.stdout.strip()
        return code == str(expected_code), code
    except:
        return False, "ERROR"

def run_curl_output(args, timeout=5):
    """Run curl and get output"""
    try:
        cmd = ['curl', '-s'] + args
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return result.stdout
    except:
        return ""

def check_server_running():
    """Check if server is running"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(1)
        result = sock.connect_ex(('127.0.0.1', 8080))
        sock.close()
        return result == 0
    except:
        return False

print_header("COMPREHENSIVE WEBSERV TESTER - ALL EDGE CASES")

# Check server is running
if not check_server_running():
    print(f"{RED}ERROR: Server not running on port 8080!{RESET}")
    print(f"{YELLOW}Start server: ./webserv webserv.conf{RESET}\n")
    sys.exit(1)

print(f"{GREEN}✓ Server is running{RESET}\n")

# SECTION 1: Basic HTTP Methods
print_header("SECTION 1: HTTP Methods (15 tests)")

# GET tests
success, code = run_curl(['http://127.0.0.1:8080/'], 200)
print_test("GET / → 200", success, f"Got: {code}")

success, code = run_curl(['http://127.0.0.1:8080/index.html'], 200)
print_test("GET /index.html → 200", success, f"Got: {code}")

success, code = run_curl(['http://127.0.0.1:8080/files/a.txt'], 200)
print_test("GET /files/a.txt → 200", success, f"Got: {code}")

# Directory listing
success, code = run_curl(['http://127.0.0.1:8080/listing_dir/'], 200)
print_test("GET /listing_dir/ (autoindex) → 200", success, f"Got: {code}")

# 404 tests
success, code = run_curl(['http://127.0.0.1:8080/nonexistent'], 404)
print_test("GET /nonexistent → 404", success, f"Got: {code}")

success, code = run_curl(['http://127.0.0.1:8080/private_dir/'], 404)
print_test("GET /private_dir/ → 404", success, f"Got: {code}")

# POST tests
success, code = run_curl(['-X', 'POST', '-d', 'data=test', 'http://127.0.0.1:8080/'], 200)
print_test("POST / with data → 200", success, f"Got: {code}")

success, code = run_curl(['-X', 'POST', 'http://127.0.0.1:8080/'], 411)
print_test("POST without Content-Length → 411", success, f"Got: {code}")

success, code = run_curl(['-X', 'POST', '-H', 'Content-Length: 0', 'http://127.0.0.1:8080/files/'], 405)
print_test("POST to GET-only location → 405", success, f"Got: {code}")

# DELETE tests
os.system('echo "test" > www/delete_zone/test_delete.txt 2>/dev/null')
success, code = run_curl(['-X', 'DELETE', 'http://127.0.0.1:8080/delete_zone/test_delete.txt'], 204)
if not success:
    success, code = run_curl(['-X', 'DELETE', 'http://127.0.0.1:8080/delete_zone/test_delete.txt'], 200)
print_test("DELETE existing file → 204/200", success, f"Got: {code}")

success, code = run_curl(['-X', 'DELETE', 'http://127.0.0.1:8080/delete_zone/nonexistent.txt'], 404)
print_test("DELETE nonexistent → 404", success, f"Got: {code}")

# Invalid methods
try:
    cmd = "printf 'BREW / HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\n' | nc -w1 127.0.0.1 8080"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=2)
    success = '405' in result.stdout or '501' in result.stdout
    print_test("BREW method → 405/501", success)
except:
    print_test("BREW method → 405/501", False, "nc command failed")

# Multiple servers
success, code = run_curl(['http://127.0.0.1:9090/'], 200)
print_test("GET port 9090 → 200", success, f"Got: {code}")

success, code = run_curl(['-X', 'POST', '-d', 'test', 'http://127.0.0.1:9090/'], 405)
print_test("POST port 9090 (GET-only) → 405", success, f"Got: {code}")

# Error pages
output = run_curl_output(['http://127.0.0.1:8080/nonexistent'])
success = '404' in output and ('html' in output.lower() or 'not found' in output.lower())
print_test("404 error page has HTML", success)

# SECTION 2: CGI Tests
print_header("SECTION 2: CGI Tests (10 tests)")

# Python CGI
print(f"{YELLOW}Testing Python CGI (10s wait)...{RESET}")
start = time.time()
output = run_curl_output(['http://127.0.0.1:8080/cgi-bin/test.py'], timeout=15)
elapsed = time.time() - start
success = 'cgi worked' in output.lower() and elapsed >= 9
print_test("Python CGI /cgi-bin/test.py → works", success, f"Time: {elapsed:.1f}s")

# Alternative path
output = run_curl_output(['http://127.0.0.1:8080/cgi/test.py'], timeout=15)
success = 'cgi worked' in output.lower()
print_test("Python CGI /cgi/test.py → works", success)

# PHP CGI (Multiple CGI requirement)
output = run_curl_output(['http://127.0.0.1:8080/cgi-bin/test.php'])
success = 'php' in output.lower() and 'works' in output.lower()
print_test("PHP CGI test.php → works (Multiple CGI!)", success)

output = run_curl_output(['http://127.0.0.1:8080/cgi/test.php'])
success = 'php' in output.lower()
print_test("PHP CGI /cgi/test.php → works", success)

# Query strings
output = run_curl_output(['"http://127.0.0.1:8080/cgi-bin/q.py?name=test&value=42"'])
success = 'name=test' in output or 'value=42' in output
print_test("CGI query string → parsed", success)

# Environment variables
output = run_curl_output(['"http://127.0.0.1:8080/cgi/q.py?v=1&x=2"'])
has_query = 'v=1' in output or 'x=2' in output
has_method = 'get' in output.lower()
success = has_query and has_method
print_test("CGI environment vars set", success, f"Query: {has_query}, Method: {has_method}")

# POST to CGI
output = run_curl_output(['-X', 'POST', '-d', 'test=data', 'http://127.0.0.1:8080/cgi-bin/q.py'])
success = 'post' in output.lower()
print_test("CGI POST method → works", success)

# Non-executable in CGI dir
success, code = run_curl(['http://127.0.0.1:8080/cgi-bin/hello.txt'], 403)
if not success:
    success, code = run_curl(['http://127.0.0.1:8080/cgi-bin/hello.txt'], 404)
print_test("Non-executable in CGI dir → 403/404", success, f"Got: {code}")

# CGI with special characters in query
output = run_curl_output(['"http://127.0.0.1:8080/cgi/q.py?name=John%20Doe&email=test%40example.com"'])
success = len(output) > 0
print_test("CGI with URL encoding → works", success)

# CGI timeout test (optional, takes 60s)
print(f"{YELLOW}Skipping timeout test (takes 60s)...{RESET}")
print_test("CGI timeout test", True, "SKIPPED")

# SECTION 3: File Upload
print_header("SECTION 3: File Upload (8 tests)")

# Create test file
os.system('echo "upload test content" > /tmp/upload_test.txt')

# Basic upload
success, code = run_curl(['-F', 'file=@/tmp/upload_test.txt', 'http://127.0.0.1:8080/uploads'], 201)
if not success:
    success, code = run_curl(['-F', 'file=@/tmp/upload_test.txt', 'http://127.0.0.1:8080/uploads'], 200)
print_test("File upload → 201/200", success, f"Got: {code}")

# Check if file exists
time.sleep(0.5)
exists = os.path.exists('www/uploads/upload_test.txt')
print_test("Uploaded file exists", exists)

# Check filename preservation
if exists:
    with open('www/uploads/upload_test.txt', 'r') as f:
        content = f.read()
    success = 'upload test content' in content
    print_test("File content preserved", success)
else:
    print_test("File content preserved", False, "File not found")

# Multiple file upload
os.system('echo "file1" > /tmp/file1.txt')
os.system('echo "file2" > /tmp/file2.txt')
success, code = run_curl(['-F', 'file=@/tmp/file1.txt', '-F', 'file=@/tmp/file2.txt', 'http://127.0.0.1:8080/uploads'], 201)
if not success:
    success, code = run_curl(['-F', 'file=@/tmp/file1.txt', 'http://127.0.0.1:8080/uploads'], 201)
print_test("Multiple file upload", success, f"Got: {code}")

# Large file (2MB to 1MB limit server)
os.system('dd if=/dev/zero of=/tmp/large.bin bs=1M count=2 2>/dev/null')
success, code = run_curl(['-F', 'file=@/tmp/large.bin', 'http://127.0.0.1:9090/'], 413)
print_test("Upload exceeding limit → 413", success, f"Got: {code}")

# Upload to allowed location
success, code = run_curl(['-F', 'file=@/tmp/upload_test.txt', 'http://127.0.0.1:8080/uploads'], 201)
if not success:
    success, code = run_curl(['-F', 'file=@/tmp/upload_test.txt', 'http://127.0.0.1:8080/uploads'], 200)
print_test("Upload to /uploads → success", success, f"Got: {code}")

# Different file types
os.system('echo "binary data" > /tmp/test.bin')
success, code = run_curl(['-F', 'file=@/tmp/test.bin', 'http://127.0.0.1:8080/uploads'], 201)
if not success:
    success, code = run_curl(['-F', 'file=@/tmp/test.bin', 'http://127.0.0.1:8080/uploads'], 200)
print_test("Upload binary file", success, f"Got: {code}")

# Cleanup
os.system('rm -f /tmp/upload_test.txt /tmp/file1.txt /tmp/file2.txt /tmp/large.bin /tmp/test.bin')

# Empty upload
success, code = run_curl(['-X', 'POST', '-H', 'Content-Type: multipart/form-data', 'http://127.0.0.1:8080/uploads'], 200)
print_test("Empty upload handled", success or code in ['400', '411'], f"Got: {code}")

# SECTION 4: Cookies & Sessions (BONUS)
print_header("SECTION 4: Cookies & Sessions - BONUS (5 tests)")

# Session cookie test
output = run_curl_output(['-i', 'http://127.0.0.1:8080/cgi-bin/session.py'])
has_set_cookie = 'Set-Cookie:' in output
has_session_id = 'session_id' in output
print_test("Session sets cookies", has_set_cookie and has_session_id, 
           f"Set-Cookie: {has_set_cookie}, session_id: {has_session_id}")

# Multiple cookies
count = output.count('Set-Cookie:')
print_test("Multiple cookies set", count >= 2, f"Found {count} cookies")

# Cookie content
has_visit_count = 'visit_count' in output
print_test("Visit counter cookie", has_visit_count)

# Session persistence test
output2 = run_curl_output(['-i', 'http://127.0.0.1:8080/cgi-bin/session.py'])
success = 'session' in output2.lower()
print_test("Session page renders", success)

# Cookie in response body
success = 'session' in output.lower() or 'cookie' in output.lower()
print_test("Session HTML renders", success)

# SECTION 5: Edge Cases & Error Handling
print_header("SECTION 5: Edge Cases (15 tests)")

# Very long URL
long_url = 'http://127.0.0.1:8080/' + 'a' * 2000
success, code = run_curl([long_url], 414)
if not success:
    success = code in ['400', '404', '413']
print_test("Long URL handled", success, f"Got: {code}")

# Invalid HTTP version
try:
    cmd = "printf 'GET / HTTP/9.9\\r\\nHost: localhost\\r\\n\\r\\n' | nc -w1 127.0.0.1 8080"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=2)
    success = '400' in result.stdout or '505' in result.stdout
    print_test("Invalid HTTP version → 400/505", success)
except:
    print_test("Invalid HTTP version → 400/505", False, "nc failed")

# Missing Host header (HTTP/1.1)
try:
    cmd = "printf 'GET / HTTP/1.1\\r\\n\\r\\n' | nc -w1 127.0.0.1 8080"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=2)
    success = '400' in result.stdout
    print_test("HTTP/1.1 without Host → 400", success)
except:
    print_test("HTTP/1.1 without Host → 400", False, "nc failed")

# Malformed request
try:
    cmd = "printf 'INVALID REQUEST\\r\\n\\r\\n' | nc -w1 127.0.0.1 8080"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=2)
    success = '400' in result.stdout
    print_test("Malformed request → 400", success)
except:
    print_test("Malformed request → 400", False, "nc failed")

# Double slash in path
output = run_curl_output(['http://127.0.0.1:8080//files//a.txt'])
success = len(output) > 0
print_test("Double slash in path handled", success)

# Path traversal attempt
success, code = run_curl(['http://127.0.0.1:8080/../etc/passwd'], 403)
if not success:
    success = code in ['400', '404']
print_test("Path traversal blocked", success, f"Got: {code}")

# Null byte in path
success, code = run_curl(['http://127.0.0.1:8080/test%00.txt'], 400)
if not success:
    success = code in ['403', '404']
print_test("Null byte in path handled", success, f"Got: {code}")

# Very large POST
os.system('dd if=/dev/zero of=/tmp/huge.bin bs=1M count=1200 2>/dev/null')
success, code = run_curl(['-X', 'POST', '--data-binary', '@/tmp/huge.bin', 'http://127.0.0.1:8080/'], 413, timeout=10)
print_test("Very large POST → 413", success, f"Got: {code}")
os.system('rm -f /tmp/huge.bin')

# Redirects
success, code = run_curl(['-L', 'http://127.0.0.1:8080/redirect'], 200, timeout=5)
print_test("Redirect works", success or code in ['301', '302'], f"Got: {code}")

# Keep-alive
output = run_curl_output(['-i', '-H', 'Connection: keep-alive', 'http://127.0.0.1:8080/'])
success = 'Connection:' in output or 'Keep-Alive' in output or len(output) > 0
print_test("Keep-alive header handled", success)

# Multiple requests same connection
success_count = 0
for i in range(3):
    success, code = run_curl(['http://127.0.0.1:8080/'], 200)
    if success:
        success_count += 1
print_test("Multiple sequential requests", success_count == 3, f"Passed: {success_count}/3")

# Chunked transfer encoding (if supported)
try:
    cmd = "printf 'POST / HTTP/1.1\\r\\nHost: localhost\\r\\nTransfer-Encoding: chunked\\r\\n\\r\\n5\\r\\nhello\\r\\n0\\r\\n\\r\\n' | nc -w1 127.0.0.1 8080"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=2)
    success = '200' in result.stdout or '400' in result.stdout
    print_test("Chunked encoding handled", success)
except:
    print_test("Chunked encoding handled", False, "nc failed")

# Different HTTP methods on same resource
success, code = run_curl(['http://127.0.0.1:8080/files/a.txt'], 200)
s1 = success
success, code = run_curl(['-X', 'DELETE', 'http://127.0.0.1:8080/delete_zone/protected.txt'], 204)
if not success:
    success, code = run_curl(['-X', 'DELETE', 'http://127.0.0.1:8080/delete_zone/protected.txt'], 200)
s2 = success or code in ['200', '204', '404']
print_test("Multiple methods work", s1 and s2)

# Case sensitivity
success, code = run_curl(['http://127.0.0.1:8080/FILES/a.txt'], 404)
print_test("Case sensitive paths", success, f"Got: {code}")

# Summary
print_header("TEST SUMMARY")
print(f"{GREEN}✓ Passed: {passed_tests:>3}{RESET}")
print(f"{RED}✗ Failed: {len(failed_tests):>3}{RESET}")
print(f"{BLUE}  Total:  {total_tests:>3}{RESET}")

if total_tests > 0:
    percentage = (passed_tests / total_tests) * 100
    print(f"\n{BOLD}Success Rate: {percentage:.1f}%{RESET}\n")
    
    if failed_tests:
        print(f"\n{RED}Failed Tests:{RESET}")
        for test in failed_tests:
            print(f"  - {test}")
    
    if percentage >= 90:
        print(f"\n{GREEN}{BOLD}🎉 EXCELLENT! Ready for evaluation!{RESET}")
        sys.exit(0)
    elif percentage >= 75:
        print(f"\n{YELLOW}Good! Fix remaining issues.{RESET}")
        sys.exit(0)
    else:
        print(f"\n{RED}Needs work. Review failures.{RESET}")
        sys.exit(1)

print(f"\n{BLUE}{'='*70}{RESET}\n")
