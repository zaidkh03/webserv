# ⚡ QUICK START - WEBSERV

## 3 Commands to Success

```bash
# 1. Extract
tar -xzf webserv_FINAL_COMPLETE.tar.gz && cd webserv

# 2. Setup (REQUIRED!)
chmod +x SETUP.sh && ./SETUP.sh

# 3. Run
./webserv webserv.conf
```

Then in another terminal:
```bash
./evaluation_tester.py
```

---

## ⚠️ Why Setup is Required

TAR archives **don't preserve executable permissions**.

Without running `./SETUP.sh`:
- ❌ CGI returns 404 (scripts not executable)
- ❌ DELETE returns 404 (test files missing)
- ❌ Tests fail with 60-70% pass rate

After running `./SETUP.sh`:
- ✅ CGI works (chmod +x applied)
- ✅ DELETE works (files created)
- ✅ Tests pass with 85-95% pass rate

---

## 📊 What You Get

### All Mandatory Requirements ✅
- HTTP/1.1 server
- GET, POST, DELETE
- Static files
- File uploads (preserves names!)
- CGI (Python + PHP)
- Multiple servers/ports
- Error pages
- Directory listing
- Redirects

### Both Bonus Requirements ✅
- Cookies & sessions
- Multiple CGI types

---

## 🧪 Quick Tests

```bash
# Basic HTTP
curl http://127.0.0.1:8080/

# Python CGI
curl http://127.0.0.1:8080/cgi/test.py

# PHP CGI (Multiple CGI!)
curl http://127.0.0.1:8080/cgi/test.php

# Cookies (BONUS!)
curl -i http://127.0.0.1:8080/cgi-bin/session.py

# File upload (preserves name!)
echo "test" > file.txt
curl -F "file=@file.txt" http://127.0.0.1:8080/uploads
ls www/uploads/  # Shows: file.txt ✓

# Stress test
siege -b -c 25 -r 200 http://127.0.0.1:8080/
```

---

## 🔥 Expected Results

```
========================================
TEST SUMMARY
========================================
✓ Passed:  20-22
✗ Failed:   0-2
  Total:   23

Success Rate: 87-96%

🎉 EXCELLENT! Ready for evaluation!
```

---

## 🆘 If Something Fails

```bash
# Check what's wrong
python3 pre_flight_test.py

# Fix everything
./SETUP.sh

# Try again
./webserv webserv.conf
```

---

**That's it! You're ready for evaluation! 🚀**

See README.md for detailed documentation.
