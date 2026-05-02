#!/usr/bin/env python3
"""
Pre-Flight Test - Verify everything before evaluation
Tests all components without requiring server to be running
"""

import os
import sys
import subprocess

GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'

def print_header(text):
    print(f"\n{BLUE}{'='*70}{RESET}")
    print(f"{BLUE}{text}{RESET}")
    print(f"{BLUE}{'='*70}{RESET}\n")

def check_file(path, executable=False):
    """Check if file exists and optionally if it's executable"""
    if not os.path.exists(path):
        return False, "File doesn't exist"
    
    if executable:
        if not os.access(path, os.X_OK):
            return False, "File exists but not executable"
    
    return True, "OK"

def check_file_content(path, required_content):
    """Check if file contains required content"""
    if not os.path.exists(path):
        return False, "File doesn't exist"
    
    try:
        with open(path, 'r') as f:
            content = f.read()
            for req in required_content:
                if req not in content:
                    return False, f"Missing: {req}"
        return True, "OK"
    except:
        return False, "Cannot read file"

print_header("WEBSERV PRE-FLIGHT CHECKLIST")

tests_passed = 0
tests_failed = 0

# Test 1: Build system
print(f"{YELLOW}[1] Build System{RESET}")
result, msg = check_file("Makefile")
if result:
    print(f"  {GREEN}✓{RESET} Makefile exists")
    tests_passed += 1
else:
    print(f"  {RED}✗{RESET} Makefile: {msg}")
    tests_failed += 1

result, msg = check_file("webserv", executable=True)
if result:
    print(f"  {GREEN}✓{RESET} webserv executable exists and is executable")
    tests_passed += 1
else:
    print(f"  {RED}✗{RESET} webserv: {msg}")
    print(f"     {YELLOW}FIX: Run 'make'{RESET}")
    tests_failed += 1

# Test 2: Configuration files
print(f"\n{YELLOW}[2] Configuration Files{RESET}")
result, msg = check_file_content("webserv.conf", [
    "listen 8080",
    "listen 9090",
    "location /cgi-bin",
    "location /cgi",
    "cgi .py",
    "cgi .php"
])
if result:
    print(f"  {GREEN}✓{RESET} webserv.conf has all required sections")
    tests_passed += 1
else:
    print(f"  {RED}✗{RESET} webserv.conf: {msg}")
    tests_failed += 1

result, msg = check_file("test_root/router_test.conf")
if result:
    print(f"  {GREEN}✓{RESET} test_root/router_test.conf exists")
    tests_passed += 1
else:
    print(f"  {RED}✗{RESET} test_root/router_test.conf: {msg}")
    tests_failed += 1

# Test 3: Python CGI Scripts
print(f"\n{YELLOW}[3] Python CGI Scripts{RESET}")

scripts = [
    ("cgi-bin/test.py", ["#!/usr/bin/python3", "sleep(10)", "CGI worked"]),
    ("cgi-bin/q.py", ["#!/usr/bin/python3", "QUERY_STRING", "REQUEST_METHOD"]),
    ("cgi-bin/loop.py", ["#!/usr/bin/python3", "while True"]),
    ("cgi-bin/session.py", ["#!/usr/bin/python3", "Set-Cookie", "session_id"])
]

for script, content in scripts:
    result, msg = check_file(script, executable=True)
    if result:
        result2, msg2 = check_file_content(script, content)
        if result2:
            print(f"  {GREEN}✓{RESET} {script} - executable and correct")
            tests_passed += 1
        else:
            print(f"  {RED}✗{RESET} {script} - executable but {msg2}")
            tests_failed += 1
    else:
        print(f"  {RED}✗{RESET} {script} - {msg}")
        print(f"     {YELLOW}FIX: chmod +x {script}{RESET}")
        tests_failed += 1

# Test 4: PHP CGI
print(f"\n{YELLOW}[4] PHP CGI (Multiple CGI Requirement){RESET}")
result, msg = check_file("cgi-bin/test.php", executable=True)
if result:
    result2, msg2 = check_file_content("cgi-bin/test.php", ["#!/usr/bin/php-cgi", "<?php", "PHP CGI Works"])
    if result2:
        print(f"  {GREEN}✓{RESET} cgi-bin/test.php - executable and correct")
        tests_passed += 1
    else:
        print(f"  {RED}✗{RESET} cgi-bin/test.php - executable but {msg2}")
        tests_failed += 1
else:
    print(f"  {RED}✗{RESET} cgi-bin/test.php - {msg}")
    print(f"     {YELLOW}FIX: chmod +x cgi-bin/test.php{RESET}")
    tests_failed += 1

# Test 5: WWW Structure
print(f"\n{YELLOW}[5] WWW Directory Structure{RESET}")

required_files = [
    "www/index.html",
    "www/files/a.txt",
    "www/listing_dir/file1.txt",
    "www/listing_dir/file2.txt",
    "www/delete_zone/protected.txt"
]

for f in required_files:
    result, msg = check_file(f)
    if result:
        print(f"  {GREEN}✓{RESET} {f} exists")
        tests_passed += 1
    else:
        print(f"  {RED}✗{RESET} {f} - {msg}")
        tests_failed += 1

# Test 6: private_dir should NOT exist
if not os.path.exists("www/private_dir"):
    print(f"  {GREEN}✓{RESET} www/private_dir removed (will return 404)")
    tests_passed += 1
else:
    print(f"  {YELLOW}⚠{RESET} www/private_dir exists (will return 403 instead of 404)")
    print(f"     {YELLOW}FIX: rm -rf www/private_dir{RESET}")

# Test 7: Upload directory
print(f"\n{YELLOW}[6] Upload Directory{RESET}")
if os.path.exists("www/uploads") and os.path.isdir("www/uploads"):
    print(f"  {GREEN}✓{RESET} www/uploads directory exists")
    tests_passed += 1
else:
    print(f"  {RED}✗{RESET} www/uploads directory missing")
    print(f"     {YELLOW}FIX: mkdir -p www/uploads{RESET}")
    tests_failed += 1

# Test 8: Error pages
print(f"\n{YELLOW}[7] Error Pages{RESET}")
error_pages = ["404.html", "403.html", "405.html", "411.html", "413.html", "500.html"]
missing = []
for page in error_pages:
    path = f"www/errors/{page}"
    if os.path.exists(path):
        tests_passed += 1
    else:
        missing.append(page)
        tests_failed += 1

if not missing:
    print(f"  {GREEN}✓{RESET} All error pages exist")
else:
    print(f"  {RED}✗{RESET} Missing error pages: {', '.join(missing)}")

# Test 9: Test scripts
print(f"\n{YELLOW}[8] Test Scripts{RESET}")
result, msg = check_file("evaluation_tester.py", executable=True)
if result:
    print(f"  {GREEN}✓{RESET} evaluation_tester.py executable")
    tests_passed += 1
else:
    print(f"  {RED}✗{RESET} evaluation_tester.py - {msg}")
    print(f"     {YELLOW}FIX: chmod +x evaluation_tester.py{RESET}")
    tests_failed += 1

# Test 10: Setup scripts
print(f"\n{YELLOW}[9] Setup Scripts{RESET}")
for script in ["SETUP.sh", "ULTIMATE_SETUP.sh"]:
    result, msg = check_file(script, executable=True)
    if result:
        print(f"  {GREEN}✓{RESET} {script} executable")
        tests_passed += 1
    else:
        print(f"  {YELLOW}⚠{RESET} {script} - {msg}")

# Summary
print_header("SUMMARY")
total = tests_passed + tests_failed
percentage = (tests_passed / total * 100) if total > 0 else 0

print(f"{GREEN}✓ Passed: {tests_passed}{RESET}")
print(f"{RED}✗ Failed: {tests_failed}{RESET}")
print(f"Total: {total}")
print(f"\nSuccess Rate: {percentage:.1f}%\n")

if tests_failed == 0:
    print(f"{GREEN}🎉 PERFECT! All pre-flight checks passed!{RESET}")
    print(f"\n{BLUE}Next steps:{RESET}")
    print(f"  1. ./webserv webserv.conf")
    print(f"  2. ./evaluation_tester.py")
    print("")
    sys.exit(0)
elif tests_failed <= 5:
    print(f"{YELLOW}⚠ Some issues found. Fix them before starting server.{RESET}")
    print(f"\n{BLUE}Quick fix:{RESET}")
    print(f"  chmod +x SETUP.sh && ./SETUP.sh")
    print("")
    sys.exit(1)
else:
    print(f"{RED}❌ Multiple issues found. Run setup script.{RESET}")
    print(f"\n{BLUE}Fix command:{RESET}")
    print(f"  chmod +x SETUP.sh && ./SETUP.sh")
    print("")
    sys.exit(1)
