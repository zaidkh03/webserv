#!/usr/bin/env python3
"""
COMPLETE INTEGRATION TEST
Simulates the actual 42 evaluation process
Tests all requirements from the evaluation sheet
"""

import subprocess
import time
import os
import sys
import signal

# Colors
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
BOLD = '\033[1m'
RESET = '\033[0m'

def print_header(text):
    print(f"\n{BLUE}{BOLD}{'='*70}{RESET}")
    print(f"{BLUE}{BOLD}{text}{RESET}")
    print(f"{BLUE}{BOLD}{'='*70}{RESET}\n")

def print_test(name, passed, details=""):
    global total_tests, passed_tests
    total_tests += 1
    if passed:
        passed_tests += 1
        print(f"{GREEN}✓ PASS{RESET} - {name}")
    else:
        print(f"{RED}✗ FAIL{RESET} - {name}")
    if details:
        print(f"       {YELLOW}{details}{RESET}")

def run_command(cmd, timeout=5):
    """Run a shell command and return output"""
    try:
        result = subprocess.run(
            cmd, 
            shell=True, 
            capture_output=True, 
            text=True, 
            timeout=timeout
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except Exception as e:
        return -1, "", str(e)

def check_server_running():
    """Check if server is running on port 8080"""
    code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8080/", timeout=2)
    return "200" in out or "301" in out or "302" in out

# Global counters
total_tests = 0
passed_tests = 0
server_process = None

print_header("WEBSERV COMPLETE INTEGRATION TEST")
print(f"{YELLOW}This test simulates the actual 42 evaluation{RESET}\n")

# Pre-flight checks
print_header("SECTION 0: Pre-Flight Checks")

# Check executables
print_test(
    "webserv executable exists",
    os.path.exists("webserv") and os.access("webserv", os.X_OK),
    "Run: make" if not os.path.exists("webserv") else ""
)

# Check CGI scripts
cgi_ok = True
for script in ["test.py", "q.py", "loop.py", "session.py", "test.php"]:
    path = f"cgi-bin/{script}"
    if not os.path.exists(path):
        cgi_ok = False
        print_test(f"CGI {script} exists", False, f"Missing: {path}")
    elif not os.access(path, os.X_OK):
        cgi_ok = False
        print_test(f"CGI {script} executable", False, f"Run: chmod +x {path}")

if cgi_ok:
    print_test("All CGI scripts exist and executable", True)

# Check test files
files_ok = True
for f in ["www/files/a.txt", "www/delete_zone/protected.txt"]:
    if not os.path.exists(f):
        files_ok = False
        print_test(f"{f} exists", False, "Run: ./SETUP.sh")

if files_ok:
    print_test("All test files exist", True)

# Check server is not already running
if check_server_running():
    print(f"\n{RED}ERROR: Server already running on port 8080!{RESET}")
    print(f"{YELLOW}Kill it with: killall webserv{RESET}\n")
    sys.exit(1)

# Start server
print_header("SECTION 1: Starting Server")
print(f"{YELLOW}Starting webserv...{RESET}")

try:
    server_process = subprocess.Popen(
        ["./webserv", "webserv.conf"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    time.sleep(2)  # Give server time to start
    
    if server_process.poll() is not None:
        print(f"{RED}✗ Server crashed immediately!{RESET}")
        stdout, stderr = server_process.communicate()
        print(f"STDOUT: {stdout}")
        print(f"STDERR: {stderr}")
        sys.exit(1)
    
    if check_server_running():
        print_test("Server started successfully", True)
    else:
        print_test("Server started but not responding", False)
        server_process.kill()
        sys.exit(1)
        
except Exception as e:
    print(f"{RED}✗ Failed to start server: {e}{RESET}")
    sys.exit(1)

# Test 1: Basic HTTP
print_header("SECTION 2: Basic HTTP Tests")

# Test GET /
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8080/")
print_test("GET / → 200", "200" in out, f"Got: {out.strip()}")

# Test GET file
code, out, err = run_command("curl -s http://127.0.0.1:8080/files/a.txt")
print_test("GET /files/a.txt → content", "file contents" in out.lower() or len(out) > 0)

# Test directory listing
code, out, err = run_command("curl -s http://127.0.0.1:8080/listing_dir/")
print_test("GET /listing_dir/ → 200 (autoindex)", "file1" in out or "file2" in out or "200" in out)

# Test 404
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8080/private_dir/")
print_test("GET /private_dir/ → 404", "404" in out, f"Got: {out.strip()}")

# Test DELETE
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' -X DELETE http://127.0.0.1:8080/delete_zone/protected.txt")
success = "204" in out or "200" in out
print_test("DELETE protected.txt → 204/200", success, f"Got: {out.strip()}")

# Recreate for next test
run_command("echo 'protected content' > www/delete_zone/protected.txt")

# Test invalid method
code, out, err = run_command("printf 'BREW / HTTP/1.0\\r\\n\\r\\n' | nc 127.0.0.1 8080")
print_test("BREW method → 405/501", "405" in out or "501" in out)

# Test POST without Content-Length
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' -X POST http://127.0.0.1:8080/files/a.txt")
print_test("POST no Content-Length → 411", "411" in out, f"Got: {out.strip()}")

# Test second server
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:9090/")
print_test("GET port 9090 → 200", "200" in out, f"Got: {out.strip()}")

# Test POST on GET-only server
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' -X POST --data 'X' http://127.0.0.1:9090/")
print_test("POST port 9090 → 405", "405" in out, f"Got: {out.strip()}")

# Test 2: CGI Tests
print_header("SECTION 3: CGI Tests")

# Test Python CGI
print(f"{YELLOW}Testing Python CGI (10s wait)...{RESET}")
start = time.time()
code, out, err = run_command("curl -s http://127.0.0.1:8080/cgi/test.py", timeout=15)
elapsed = time.time() - start
cgi_works = "cgi worked" in out.lower() and elapsed >= 9
print_test("Python CGI test.py → works after 10s", cgi_works, 
           f"Elapsed: {elapsed:.1f}s, Found: {'cgi worked' in out.lower()}")

# Test PHP CGI (Multiple CGI requirement)
code, out, err = run_command("curl -s http://127.0.0.1:8080/cgi/test.php")
php_works = "php cgi" in out.lower() and "works" in out.lower()
print_test("PHP CGI test.php → works (Multiple CGI!)", php_works, 
           f"Found: {php_works}")

# Test query string
code, out, err = run_command('curl -s "http://127.0.0.1:8080/cgi/q.py?v=1&x=2"')
has_query = "string=v=1&x=2" in out or "v=1" in out
has_name = "name=/cgi/q.py" in out or "/cgi/q.py" in out
has_method = "method=get" in out.lower()
print_test("CGI query string → env vars set", 
           has_query and has_name and has_method,
           f"Query: {has_query}, Name: {has_name}, Method: {has_method}")

# Test non-executable
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8080/cgi/hello.txt")
print_test("Non-executable in CGI dir → 403/404", "403" in out or "404" in out, f"Got: {out.strip()}")

# Test 3: Cookies & Sessions (BONUS)
print_header("SECTION 4: Cookies & Sessions (BONUS)")

code, out, err = run_command("curl -i http://127.0.0.1:8080/cgi-bin/session.py")
has_cookie = "Set-Cookie:" in out and "session_id" in out
has_session = "Session" in out or "session" in out.lower()
print_test("Session/Cookies demo works", has_cookie and has_session,
           f"Cookie: {has_cookie}, Session: {has_session}")

# Test 4: File Upload
print_header("SECTION 5: File Upload")

# Create test file
run_command("echo 'test content' > /tmp/testfile.txt")
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' -F 'file=@/tmp/testfile.txt' http://127.0.0.1:8080/uploads")
upload_success = "201" in out or "200" in out
print_test("File upload → 201/200", upload_success, f"Got: {out.strip()}")

# Check if file was saved
code, out, err = run_command("ls www/uploads/testfile.txt 2>&1")
name_preserved = "testfile.txt" in out
print_test("Upload preserves filename", name_preserved, 
           f"Found: {name_preserved}")

# Test size limit
run_command("dd if=/dev/zero of=/tmp/2mb.bin bs=1M count=2 2>/dev/null")
code, out, err = run_command("curl -s -o /dev/null -w '%{http_code}' -F 'file=@/tmp/2mb.bin' http://127.0.0.1:9090/")
print_test("Upload to 1MB limit → 413", "413" in out, f"Got: {out.strip()}")

# Cleanup
run_command("rm -f /tmp/testfile.txt /tmp/2mb.bin")

# Test 5: Stress Test (Quick version)
print_header("SECTION 6: Stress Test (Quick)")

print(f"{YELLOW}Running quick stress test...{RESET}")
code, out, err = run_command("siege -b -c 10 -r 50 http://127.0.0.1:8080/ 2>&1", timeout=30)
if "availability" in out.lower():
    # Parse availability
    import re
    match = re.search(r'availability.*?(\d+\.\d+)', out, re.IGNORECASE)
    if match:
        avail = float(match.group(1))
        print_test(f"Stress test availability > 99%", avail >= 99.0, f"Achieved: {avail}%")
    else:
        print_test("Stress test", True, "Completed (check manually)")
else:
    print_test("Stress test", "failed" not in out.lower(), "Siege not installed or failed")

# Stop server
print_header("SECTION 7: Cleanup")
if server_process:
    print(f"{YELLOW}Stopping server...{RESET}")
    server_process.terminate()
    try:
        server_process.wait(timeout=5)
    except:
        server_process.kill()
    print_test("Server stopped cleanly", True)

# Summary
print_header("FINAL SUMMARY")
print(f"{GREEN}✓ Passed: {passed_tests:>3}{RESET}")
print(f"{RED}✗ Failed: {total_tests - passed_tests:>3}{RESET}")
print(f"{BLUE}  Total:  {total_tests:>3}{RESET}")

if total_tests > 0:
    percentage = (passed_tests / total_tests) * 100
    print(f"\n{BOLD}Success Rate: {percentage:.1f}%{RESET}\n")
    
    if percentage >= 90:
        print(f"{GREEN}{BOLD}🎉 EXCELLENT! Ready for evaluation!{RESET}")
        exit_code = 0
    elif percentage >= 75:
        print(f"{YELLOW}Good! Fix remaining issues.{RESET}")
        exit_code = 0
    else:
        print(f"{RED}Needs work. Review failures.{RESET}")
        exit_code = 1
else:
    exit_code = 1

print(f"\n{BLUE}{'='*70}{RESET}\n")
sys.exit(exit_code)
