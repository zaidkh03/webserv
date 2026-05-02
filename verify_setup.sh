#!/bin/bash
# Quick setup verification for Webserv

echo "=================================="
echo "Webserv Setup Verification"
echo "=================================="
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check compilation
echo -n "Checking if webserv is compiled... "
if [ -f "webserv" ]; then
    echo -e "${GREEN}✓ Found${NC}"
else
    echo -e "${RED}✗ Not found${NC}"
    echo "Run: make"
    exit 1
fi

# Check config
echo -n "Checking configuration file... "
if [ -f "webserv.conf" ]; then
    echo -e "${GREEN}✓ Found${NC}"
else
    echo -e "${RED}✗ Not found${NC}"
    exit 1
fi

# Check test script
echo -n "Checking test script... "
if [ -f "complete_test.py" ] && [ -x "complete_test.py" ]; then
    echo -e "${GREEN}✓ Found and executable${NC}"
elif [ -f "complete_test.py" ]; then
    echo -e "${YELLOW}⚠ Found but not executable${NC}"
    chmod +x complete_test.py
    echo "  Fixed: Made executable"
else
    echo -e "${RED}✗ Not found${NC}"
    exit 1
fi

# Check directory structure
echo ""
echo "Checking directory structure:"

dirs=("www" "www/files" "www/listing_dir" "www/delete_zone" "www/uploads" "www/errors" "cgi-bin" "src" "include")

for dir in "${dirs[@]}"; do
    echo -n "  $dir... "
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo "    Creating $dir"
        mkdir -p "$dir"
    fi
done

# Check test files
echo ""
echo "Checking test files:"

# www files
echo -n "  www/index.html... "
[ -f "www/index.html" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠ Missing${NC}"

echo -n "  www/files/a.txt... "
if [ -f "www/files/a.txt" ]; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${YELLOW}⚠ Creating${NC}"
    echo "file contents" > www/files/a.txt
fi

echo -n "  www/listing_dir/file1.txt... "
if [ -f "www/listing_dir/file1.txt" ]; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${YELLOW}⚠ Creating${NC}"
    echo "test file 1" > www/listing_dir/file1.txt
fi

echo -n "  www/listing_dir/file2.txt... "
if [ -f "www/listing_dir/file2.txt" ]; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${YELLOW}⚠ Creating${NC}"
    echo "test file 2" > www/listing_dir/file2.txt
fi

echo -n "  www/delete_zone/protected.txt... "
if [ -f "www/delete_zone/protected.txt" ]; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${YELLOW}⚠ Creating${NC}"
    echo "protected content" > www/delete_zone/protected.txt
fi

# CGI files
echo -n "  cgi-bin/test.py... "
if [ -f "cgi-bin/test.py" ] && [ -x "cgi-bin/test.py" ]; then
    echo -e "${GREEN}✓${NC}"
elif [ -f "cgi-bin/test.py" ]; then
    echo -e "${YELLOW}⚠ Not executable${NC}"
    chmod +x cgi-bin/test.py
    echo "    Fixed"
else
    echo -e "${RED}✗ Missing${NC}"
fi

echo -n "  cgi-bin/q.py... "
if [ -f "cgi-bin/q.py" ] && [ -x "cgi-bin/q.py" ]; then
    echo -e "${GREEN}✓${NC}"
elif [ -f "cgi-bin/q.py" ]; then
    echo -e "${YELLOW}⚠ Not executable${NC}"
    chmod +x cgi-bin/q.py
    echo "    Fixed"
else
    echo -e "${RED}✗ Missing${NC}"
fi

echo -n "  cgi-bin/loop.py... "
if [ -f "cgi-bin/loop.py" ] && [ -x "cgi-bin/loop.py" ]; then
    echo -e "${GREEN}✓${NC}"
elif [ -f "cgi-bin/loop.py" ]; then
    echo -e "${YELLOW}⚠ Not executable${NC}"
    chmod +x cgi-bin/loop.py
    echo "    Fixed"
else
    echo -e "${RED}✗ Missing${NC}"
fi

echo -n "  cgi-bin/hello.txt... "
if [ -f "cgi-bin/hello.txt" ]; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${YELLOW}⚠ Creating${NC}"
    echo "This is not a CGI script" > cgi-bin/hello.txt
fi

# Error pages
echo -n "  www/errors/404.html... "
[ -f "www/errors/404.html" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠ Missing${NC}"

echo -n "  www/errors/403.html... "
[ -f "www/errors/403.html" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠ Missing${NC}"

echo -n "  www/errors/405.html... "
[ -f "www/errors/405.html" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠ Missing${NC}"

echo -n "  www/errors/411.html... "
[ -f "www/errors/411.html" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠ Missing${NC}"

echo -n "  www/errors/413.html... "
[ -f "www/errors/413.html" ] && echo -e "${GREEN}✓${NC}" || echo -e "${YELLOW}⚠ Missing${NC}"

# Check if port is available
echo ""
echo "Checking port availability:"
echo -n "  Port 8080... "
if lsof -Pi :8080 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo -e "${RED}✗ In use${NC}"
    echo "    Kill the process using: lsof -ti:8080 | xargs kill -9"
else
    echo -e "${GREEN}✓ Available${NC}"
fi

echo -n "  Port 9090... "
if lsof -Pi :9090 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo -e "${RED}✗ In use${NC}"
    echo "    Kill the process using: lsof -ti:9090 | xargs kill -9"
else
    echo -e "${GREEN}✓ Available${NC}"
fi

# Check dependencies
echo ""
echo "Checking dependencies:"

echo -n "  curl... "
if command -v curl &> /dev/null; then
    echo -e "${GREEN}✓ Installed${NC}"
else
    echo -e "${RED}✗ Not installed${NC}"
    echo "    Install: sudo apt-get install curl (Ubuntu) or brew install curl (macOS)"
fi

echo -n "  netcat (nc)... "
if command -v nc &> /dev/null; then
    echo -e "${GREEN}✓ Installed${NC}"
else
    echo -e "${RED}✗ Not installed${NC}"
    echo "    Install: sudo apt-get install netcat (Ubuntu) or brew install netcat (macOS)"
fi

echo -n "  python3... "
if command -v python3 &> /dev/null; then
    echo -e "${GREEN}✓ Installed${NC}"
else
    echo -e "${RED}✗ Not installed${NC}"
    echo "    Install: sudo apt-get install python3 (Ubuntu) or brew install python3 (macOS)"
fi

echo -n "  siege (optional)... "
if command -v siege &> /dev/null; then
    echo -e "${GREEN}✓ Installed${NC}"
else
    echo -e "${YELLOW}⚠ Not installed (optional for stress tests)${NC}"
    echo "    Install: sudo apt-get install siege (Ubuntu) or brew install siege (macOS)"
fi

echo ""
echo "=================================="
echo -e "${GREEN}Setup verification complete!${NC}"
echo "=================================="
echo ""
echo "Next steps:"
echo "  1. Start server:  ./webserv webserv.conf"
echo "  2. Run tests:     ./complete_test.py"
echo ""
