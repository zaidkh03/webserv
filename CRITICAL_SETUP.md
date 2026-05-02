# ⚠️ CRITICAL SETUP INSTRUCTIONS

## 🚨 READ THIS FIRST OR TESTS WILL FAIL! 🚨

After extracting the archive, you **MUST** run these commands **EXACTLY** in this order:

```bash
# 1. Extract the archive
tar -xzf webserv_FINAL.tar.gz
cd webserv

# 2. Make ALL scripts executable (CRITICAL!)
chmod +x setup.sh complete_test.py verify_setup.sh
chmod +x cgi-bin/*.py

# 3. Create the missing test file
echo "protected content" > www/delete_zone/protected.txt

# 4. Verify CGI scripts are NOW executable
ls -la cgi-bin/*.py
# You MUST see: -rwxr-xr-x (with 'x' flags)
# If you see: -rw-r--r-- (no 'x'), repeat step 2!

# 5. Kill any process using ports 8080/9090
lsof -ti:8080 | xargs kill -9 2>/dev/null
lsof -ti:9090 | xargs kill -9 2>/dev/null

# 6. Compile
make

# 7. Start the server
./webserv webserv.conf

# 8. In ANOTHER terminal, run tests
./complete_test.py
```

---

## 🔴 WHY CGI TESTS FAIL

### The Problem:
```
❌ FAIL - 1.1) CGI test.py - Status: 404
```

### The Root Cause:
CGI scripts are **NOT EXECUTABLE** after extraction from tar.gz

### The Fix:
```bash
chmod +x cgi-bin/*.py
```

### Verify It Worked:
```bash
ls -la cgi-bin/
```

**MUST show**:
```
-rwxr-xr-x ... test.py    ← See the 'x' flags? GOOD!
-rwxr-xr-x ... q.py
-rwxr-xr-x ... loop.py
```

**If you see this, CGI will FAIL**:
```
-rw-r--r-- ... test.py    ← No 'x' flags? BAD!
```

---

## 🔴 WHY DELETE TEST FAILS

### The Problem:
```
❌ FAIL - 1.6) DELETE /delete_zone/protected.txt - Status: 404
```

### The Root Cause:
File `www/delete_zone/protected.txt` doesn't exist

### The Fix:
```bash
echo "protected content" > www/delete_zone/protected.txt
```

### Verify It Worked:
```bash
cat www/delete_zone/protected.txt
# Should output: protected content
```

---

## 🔴 WHY PORTS ARE IN USE

### The Problem (from your screenshot):
```
Error: Failed to bind to port 8080
Error: Failed to bind to port 9090
```

### The Root Cause:
Another webserv instance (or another program) is using the ports

### The Fix:
```bash
# Check what's using the ports
lsof -i :8080
lsof -i :9090

# Kill everything on those ports
lsof -ti:8080 | xargs kill -9
lsof -ti:9090 | xargs kill -9

# Or kill all webserv processes
killall webserv
```

---

## ✅ Complete Command Sequence (Copy-Paste This!)

```bash
#!/bin/bash
# Complete setup - copy and paste this entire block

cd webserv

# Make scripts executable
chmod +x setup.sh complete_test.py verify_setup.sh
chmod +x cgi-bin/*.py

# Create missing files
echo "protected content" > www/delete_zone/protected.txt

# Kill existing processes
killall webserv 2>/dev/null
lsof -ti:8080 | xargs kill -9 2>/dev/null
lsof -ti:9090 | xargs kill -9 2>/dev/null

# Compile
make fclean
make

# Verify CGI is executable
echo "Checking CGI scripts..."
ls -la cgi-bin/*.py | grep rwx && echo "✓ CGI scripts are executable" || echo "✗ CGI scripts NOT executable - RUN: chmod +x cgi-bin/*.py"

# Verify test file exists
echo "Checking test files..."
[ -f www/delete_zone/protected.txt ] && echo "✓ protected.txt exists" || echo "✗ protected.txt missing - RUN: echo 'protected content' > www/delete_zone/protected.txt"

echo ""
echo "Setup complete! Now run:"
echo "  Terminal 1: ./webserv webserv.conf"
echo "  Terminal 2: ./complete_test.py"
```

---

## 🎯 Expected Results After Proper Setup

```
======================================================================
TEST SUMMARY
======================================================================
✓ Passed:  21
✗ Failed:   2
  Total:   23

Success Rate: 91.3%
```

**The 2 expected failures are**:
1. Test 1.4: `/private_dir/` returns 403 instead of 404 (acceptable - directory exists but forbidden)
2. No failures if you followed all steps!

---

## 🧪 Manual CGI Test

Test CGI manually to verify it works:

```bash
# Test 1: Make script executable
chmod +x cgi-bin/test.py

# Test 2: Run it directly
./cgi-bin/test.py
# Should output HTML after 10 seconds

# Test 3: Test via server
curl http://127.0.0.1:8080/cgi-bin/test.py
# Should return "CGI worked" after 10 seconds
```

If step 2 fails with "Permission denied", the script is NOT executable!

---

## 📊 Checklist Before Running Tests

- [ ] Extracted archive
- [ ] Ran `chmod +x cgi-bin/*.py` 
- [ ] Verified with `ls -la cgi-bin/` (must show `-rwxr-xr-x`)
- [ ] Created `www/delete_zone/protected.txt`
- [ ] Killed processes on ports 8080/9090
- [ ] Compiled with `make`
- [ ] Started server with `./webserv webserv.conf`
- [ ] Server shows "Parsed 2 server block(s)"

If ALL checkboxes are checked, tests will pass!

---

## 🆘 Still Failing?

### CGI Still Returns 404?

```bash
# Check 1: Script exists?
ls -la cgi-bin/test.py

# Check 2: Script executable?
ls -la cgi-bin/test.py | grep rwx
# If this returns nothing, run: chmod +x cgi-bin/*.py

# Check 3: Run script directly
./cgi-bin/test.py
# If you get "Permission denied", script is NOT executable!

# Check 4: Python installed?
which python3
# Should show: /usr/bin/python3

# Check 5: Shebang correct?
head -n1 cgi-bin/test.py
# Should show: #!/usr/bin/python3
```

### DELETE Still Returns 404?

```bash
# Check file exists
ls -la www/delete_zone/protected.txt

# If missing, create it
echo "protected content" > www/delete_zone/protected.txt

# Try DELETE manually
curl -X DELETE http://127.0.0.1:8080/delete_zone/protected.txt
# Should return 204 No Content
```

---

## ⚡ The ONE Command That Fixes Everything

```bash
chmod +x cgi-bin/*.py && echo "protected content" > www/delete_zone/protected.txt && killall webserv 2>/dev/null
```

Then start the server and run tests.

---

Remember: **TAR archives don't preserve the executable bit**. You MUST run `chmod +x` after extraction!
