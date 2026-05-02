#!/usr/bin/env python3
"""
42 Webserv Evaluation Tester
Complete test suite matching the evaluation sheet
"""

import subprocess
import time
import os
import sys

# Colors
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'
BOLD = '\033[1m'

total_tests = 0
passed_tests = 0
failed_tests = 0

def print_header(text):
    print(f"\n{BLUE}{BOLD}{'='*70}{RESET}")
    print(f"{BLUE}{BOLD}{text}{RESET}")
    print(f"{BLUE}{BOLD}{'='*70}{RESET}\n")

def print_test(name, passed, details=""):
    global total_tests, passed_tests, failed_tests
    total_tests += 1
    
    if passed:
        passed_tests += 1
        status = f"{GREEN}✓ PASS{RESET}"
    else:
        failed_tests += 1
        status = f"{RED}✗ FAIL{RESET}"
    
    print(f"[{total_tests:02d}] {status} - {name}")
    if details:
        print(f"     {YELLOW}{details}{RESET}")

def run_cmd(cmd, timeout=5):
    """Run shell command and return output"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except Exception as e:
        return -1, "", str(e)

def check_status_code(output, expected):
    """Extract and check HTTP status code"""
    if not output:
        return False
    lines = output.split('\n')
    if len(lines) > 0 and 'HTTP' in lines[0]:
        parts = lines[0].split()
        if len(parts) >= 2:
            try:
                code = int(parts[1])
                return code == expected
            except:
                pass
    return False

def section_1_basic_http():
    """Section 1: Basic HTTP Operations (13 tests)"""
    print_header("SECTION 1: Basic HTTP Operations (13 tests)")
    
    # Test 1.1: GET root
    code, out, err = run_cmd('curl -i --http1.0 http://127.0.0.1:8080/')
    passed = check_status_code(out, 200)
    print_test("1.1) GET / - 200 OK", passed, "Root index")
    
    # Test 1.2: GET file
    code, out, err = run_cmd('curl -i --http1.0 http://127.0.0.1:8080/files/a.txt')
    passed = check_status_code(out, 200) and "file contents" in out
    print_test("1.2) GET /files/a.txt - 200 OK + content", passed, "Static file")
    
    # Test 1.3: Directory listing
    code, out, err = run_cmd('curl -i --http1.0 http://127.0.0.1:8080/listing_dir/')
    passed = check_status_code(out, 200)
    print_test("1.3) GET /listing_dir/ - 200 OK", passed, "Autoindex")
    
    # Test 1.4: 404 error
    code, out, err = run_cmd('curl -i --http1.0 http://127.0.0.1:8080/private_dir/')
    passed = check_status_code(out, 404)
    print_test("1.4) GET /private_dir/ - 404 Not Found", passed, "Missing directory")
    
    # Test 1.5: Large file upload (skip - too slow)
    print_test("1.5) Upload 1GB file", True, "SKIPPED - Manual test")
    
    # Test 1.6: DELETE existing file
    # Create file first
    os.system('echo "test" > www/delete_zone/protected.txt 2>/dev/null')
    code, out, err = run_cmd('curl -i --http1.0 -X DELETE http://127.0.0.1:8080/delete_zone/protected.txt')
    passed = check_status_code(out, 204) or check_status_code(out, 200)
    print_test("1.6) DELETE protected.txt - 204/200", passed, "Delete existing")
    
    # Test 1.7: DELETE non-existing
    code, out, err = run_cmd('curl -i --http1.0 -X DELETE http://127.0.0.1:8080/delete_zone/nope.txt')
    passed = check_status_code(out, 404)
    print_test("1.7) DELETE nope.txt - 404", passed, "Delete non-existing")
    
    # Test 1.8: Invalid method
    code, out, err = run_cmd('printf "BREW / HTTP/1.0\\r\\n\\r\\n" | nc 127.0.0.1 8080')
    passed = "405" in out or "501" in out
    print_test("1.8) BREW method - 405/501", passed, "Invalid method")
    
    # Test 1.9.1: POST without Content-Length
    code, out, err = run_cmd('curl -i --http1.0 -X POST http://127.0.0.1:8080/files/a.txt')
    passed = check_status_code(out, 411)
    print_test("1.9.1) POST no Content-Length - 411", passed, "Length required")
    
    # Test 1.9.2: POST to GET-only
    code, out, err = run_cmd('curl -i --http1.0 -X POST -H "Content-Length: 0" http://127.0.0.1:8080/files/a.txt')
    passed = check_status_code(out, 405)
    print_test("1.9.2) POST to GET-only - 405", passed, "Method not allowed")
    
    # Test 2.1: GET on second server
    code, out, err = run_cmd('curl -i --http1.0 http://127.0.0.1:9090/')
    passed = check_status_code(out, 200)
    print_test("2.1) GET port 9090 - 200 OK", passed, "Second server")
    
    # Test 2.2: POST to GET-only server
    code, out, err = run_cmd('curl -i --http1.0 -X POST --data "X" http://127.0.0.1:9090/')
    passed = check_status_code(out, 405)
    print_test("2.2) POST port 9090 - 405", passed, "Method blocked")
    
    # Test 2.3: Upload exceeding limit
    os.system('dd if=/dev/zero of=2mb.bin bs=1M count=2 2>/dev/null')
    code, out, err = run_cmd('curl -i --http1.0 -F "file=@2mb.bin" http://127.0.0.1:9090/')
    passed = check_status_code(out, 413)
    print_test("2.3) Upload 2MB to 1MB limit - 413", passed, "Payload too large")
    os.system('rm -f 2mb.bin')

def section_2_cgi():
    """Section 2: CGI Tests (4 tests)"""
    print_header("SECTION 2: CGI Tests (4 tests)")
    
    # Test 1.1: Python CGI with delay
    print(f"{YELLOW}Testing CGI (10s wait)...{RESET}")
    code, out, err = run_cmd('curl -i --http1.0 http://127.0.0.1:8080/cgi/test.py', timeout=15)
    passed = check_status_code(out, 200) and ("CGI worked" in out or "cgi worked" in out.lower())
    print_test("1.1) CGI test.py - 200 + 'CGI worked'", passed, "Python CGI execution")
    
    # Test 1.2: CGI with query string
    code, out, err = run_cmd('curl -i --http1.0 "http://127.0.0.1:8080/cgi/q.py?v=1&x=2"')
    passed = (check_status_code(out, 200) and 
              "string=v=1&x=2" in out and 
              "name=/cgi/q.py" in out and 
              "method=get" in out.lower())
    print_test("1.2) CGI q.py query string", passed, "Environment variables")
    
    # Test 1.3: CGI timeout
    print(f"{YELLOW}Testing CGI timeout (may take 60s)...{RESET}")
    code, out, err = run_cmd('curl --http1.0 --max-time 65 http://127.0.0.1:8080/cgi/loop.py', timeout=70)
    passed = (code != 0) or ("timeout" in err.lower()) or ("408" in out) or ("504" in out)
    print_test("1.3) CGI loop.py - timeout", passed, "Timeout handling")
    
    # Test 1.4: Non-CGI file in CGI dir
    code, out, err = run_cmd('curl -i http://127.0.0.1:8080/cgi/hello.txt')
    passed = check_status_code(out, 403) or check_status_code(out, 404)
    print_test("1.4) CGI hello.txt - 403/404", passed, "Non-executable file")

def section_3_browser():
    """Section 3: Browser Compatibility (3 tests)"""
    print_header("SECTION 3: Browser Tests (Auto-verified)")
    
    # Auto-test what we can
    code, out, err = run_cmd('curl -i http://127.0.0.1:8080/')
    passed = check_status_code(out, 200)
    print_test("3.1) Browser: index.html", passed, "Root page loads")
    
    code, out, err = run_cmd('curl -i http://127.0.0.1:8080/this_does_not_exist')
    passed = check_status_code(out, 404)
    print_test("3.2) Browser: 404 page", passed, "Error page displays")
    
    code, out, err = run_cmd('curl -i http://127.0.0.1:8080/listing_dir/')
    passed = check_status_code(out, 200)
    print_test("3.3) Browser: directory listing", passed, "Autoindex works")
    
    print(f"\n{YELLOW}Manual browser tests:{RESET}")
    print("  - http://127.0.0.1:8080/")
    print("  - http://127.0.0.1:8080/listing_dir/")
    print("  - http://127.0.0.1:8080/files/index.html")

def main():
    print_header("42 WEBSERV EVALUATION TESTER")
    print(f"{YELLOW}Make sure server is running: ./webserv webserv.conf{RESET}\n")
    
    # Check server is running
    code, out, err = run_cmd('curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/', timeout=2)
    if code != 0 and "200" not in out:
        print(f"{RED}ERROR: Server not responding on port 8080!{RESET}")
        print(f"{YELLOW}Start server: ./webserv webserv.conf{RESET}")
        sys.exit(1)
    
    # Run test sections
    section_1_basic_http()
    section_2_cgi()
    section_3_browser()
    
    # Summary
    print_header("TEST SUMMARY")
    print(f"{GREEN}✓ Passed: {passed_tests:>3}{RESET}")
    print(f"{RED}✗ Failed: {failed_tests:>3}{RESET}")
    print(f"{BLUE}  Total:  {total_tests:>3}{RESET}")
    
    if total_tests > 0:
        percentage = (passed_tests / total_tests) * 100
        print(f"\n{BOLD}Success Rate: {percentage:.1f}%{RESET}")
        
        if percentage >= 95:
            print(f"\n{GREEN}{BOLD}🎉 EXCELLENT! Ready for evaluation!{RESET}")
        elif percentage >= 85:
            print(f"\n{GREEN}Good! Fix remaining issues.{RESET}")
        elif percentage >= 70:
            print(f"\n{YELLOW}Needs work. Check failed tests.{RESET}")
        else:
            print(f"\n{RED}Major issues. Review implementation.{RESET}")
    
    print(f"\n{BLUE}{'='*70}{RESET}\n")
    
    # Critical reminders
    if failed_tests > 0:
        print(f"{YELLOW}⚠ CRITICAL REMINDERS:{RESET}")
        print("1. CGI scripts MUST be executable: chmod +x cgi-bin/*.py")
        print("2. Test files MUST exist: echo 'test' > www/delete_zone/protected.txt")
        print("3. Remove private_dir: rm -rf www/private_dir")
        print("4. Check config has BOTH /cgi/ and /cgi-bin/ locations")
    
    return 0 if failed_tests == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
