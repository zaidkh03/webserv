#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================="
echo "WEBSERV - COMPLETE SETUP SCRIPT"
echo "Version: FINAL"
echo "========================================="
echo ""

# Track errors
ERRORS=0

echo -e "${BLUE}[STEP 1/10] Checking directory...${NC}"
if [ ! -f "Makefile" ]; then
    echo -e "${RED}ERROR: Makefile not found. Are you in the webserv directory?${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Directory OK${NC}"
echo ""

echo -e "${BLUE}[STEP 2/10] Creating required directories...${NC}"
mkdir -p www/files www/listing_dir www/delete_zone www/uploads www/errors cgi-bin test_root 2>/dev/null
echo -e "${GREEN}✓ Directories created${NC}"
echo ""

echo -e "${BLUE}[STEP 3/10] Creating test files...${NC}"
echo "file contents" > www/files/a.txt
echo "Test file 1" > www/listing_dir/file1.txt
echo "Test file 2" > www/listing_dir/file2.txt
echo "Test file 3" > www/listing_dir/file3.txt
echo "protected content" > www/delete_zone/protected.txt
echo "test content for DELETE operations" > www/test.txt
echo -e "${GREEN}✓ Test files created${NC}"
echo ""

echo -e "${BLUE}[STEP 4/10] Making CGI scripts executable...${NC}"
if [ -d "cgi-bin" ]; then
    chmod +x cgi-bin/*.py 2>/dev/null
    chmod +x cgi-bin/*.php 2>/dev/null
    
    # Verify
    if ls cgi-bin/*.py 1> /dev/null 2>&1; then
        if ls -la cgi-bin/*.py | grep -q 'rwx'; then
            echo -e "${GREEN}✓ Python CGI scripts are executable${NC}"
        else
            echo -e "${RED}✗ Failed to make Python scripts executable${NC}"
            echo -e "${YELLOW}  Try: chmod +x cgi-bin/*.py${NC}"
            ERRORS=$((ERRORS + 1))
        fi
    fi
    
    if ls cgi-bin/*.php 1> /dev/null 2>&1; then
        if ls -la cgi-bin/*.php | grep -q 'rwx'; then
            echo -e "${GREEN}✓ PHP CGI scripts are executable${NC}"
        else
            echo -e "${RED}✗ Failed to make PHP scripts executable${NC}"
            echo -e "${YELLOW}  Try: chmod +x cgi-bin/*.php${NC}"
            ERRORS=$((ERRORS + 1))
        fi
    fi
else
    echo -e "${RED}✗ cgi-bin directory not found${NC}"
    ERRORS=$((ERRORS + 1))
fi
echo ""

echo -e "${BLUE}[STEP 5/10] Making test scripts executable...${NC}"
chmod +x *.py 2>/dev/null
chmod +x *.sh 2>/dev/null
echo -e "${GREEN}✓ Scripts made executable${NC}"
echo ""

echo -e "${BLUE}[STEP 6/10] Removing private_dir (for 404 test)...${NC}"
rm -rf www/private_dir 2>/dev/null
echo -e "${GREEN}✓ private_dir removed${NC}"
echo ""

echo -e "${BLUE}[STEP 7/10] Killing processes on ports 8080/9090...${NC}"
# Kill by port
lsof -ti:8080 | xargs kill -9 2>/dev/null
lsof -ti:9090 | xargs kill -9 2>/dev/null
# Kill by name
killall webserv 2>/dev/null
sleep 1
echo -e "${GREEN}✓ Ports cleared${NC}"
echo ""

echo -e "${BLUE}[STEP 8/10] Compiling project...${NC}"
make fclean >/dev/null 2>&1
if make 2>&1 | grep -q "webserv created"; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
else
    echo -e "${RED}✗ Compilation failed${NC}"
    echo -e "${YELLOW}  Run 'make' manually to see errors${NC}"
    ERRORS=$((ERRORS + 1))
fi
echo ""

echo -e "${BLUE}[STEP 9/10] Verifying configuration...${NC}"
if [ -f "webserv.conf" ]; then
    echo -e "${GREEN}✓ webserv.conf exists${NC}"
else
    echo -e "${RED}✗ webserv.conf missing${NC}"
    ERRORS=$((ERRORS + 1))
fi

if [ -f "test_root/router_test.conf" ]; then
    echo -e "${GREEN}✓ test_root/router_test.conf exists${NC}"
else
    echo -e "${YELLOW}⚠ test_root/router_test.conf missing (optional)${NC}"
fi
echo ""

echo -e "${BLUE}[STEP 10/10] Final verification checklist...${NC}"
echo ""
echo "Critical items:"

# Executable
if [ -x "webserv" ]; then
    echo -e "  ${GREEN}✓${NC} webserv executable exists and is executable"
else
    echo -e "  ${RED}✗${NC} webserv executable missing or not executable"
    ERRORS=$((ERRORS + 1))
fi

# CGI Python
if ls cgi-bin/*.py 1> /dev/null 2>&1; then
    if ls -la cgi-bin/*.py | grep -q 'rwx'; then
        echo -e "  ${GREEN}✓${NC} Python CGI scripts are executable"
    else
        echo -e "  ${RED}✗${NC} Python CGI scripts NOT executable"
        echo -e "     ${YELLOW}FIX: chmod +x cgi-bin/*.py${NC}"
        ERRORS=$((ERRORS + 1))
    fi
else
    echo -e "  ${YELLOW}⚠${NC} No Python CGI scripts found"
fi

# CGI PHP
if ls cgi-bin/*.php 1> /dev/null 2>&1; then
    if ls -la cgi-bin/*.php | grep -q 'rwx'; then
        echo -e "  ${GREEN}✓${NC} PHP CGI scripts are executable"
    else
        echo -e "  ${RED}✗${NC} PHP CGI scripts NOT executable"
        echo -e "     ${YELLOW}FIX: chmod +x cgi-bin/*.php${NC}"
        ERRORS=$((ERRORS + 1))
    fi
else
    echo -e "  ${YELLOW}⚠${NC} No PHP CGI scripts found"
fi

# Test files
if [ -f "www/files/a.txt" ]; then
    echo -e "  ${GREEN}✓${NC} www/files/a.txt exists"
else
    echo -e "  ${RED}✗${NC} www/files/a.txt missing"
    ERRORS=$((ERRORS + 1))
fi

if [ -f "www/delete_zone/protected.txt" ]; then
    echo -e "  ${GREEN}✓${NC} www/delete_zone/protected.txt exists"
else
    echo -e "  ${RED}✗${NC} www/delete_zone/protected.txt missing"
    ERRORS=$((ERRORS + 1))
fi

# private_dir removed?
if [ ! -d "www/private_dir" ]; then
    echo -e "  ${GREEN}✓${NC} www/private_dir removed (returns 404)"
else
    echo -e "  ${YELLOW}⚠${NC} www/private_dir exists (will return 403)"
fi

# Ports
if lsof -i :8080 >/dev/null 2>&1; then
    echo -e "  ${RED}✗${NC} Port 8080 is in use"
    echo -e "     ${YELLOW}FIX: lsof -ti:8080 | xargs kill -9${NC}"
    ERRORS=$((ERRORS + 1))
else
    echo -e "  ${GREEN}✓${NC} Port 8080 is available"
fi

if lsof -i :9090 >/dev/null 2>&1; then
    echo -e "  ${RED}✗${NC} Port 9090 is in use"
    echo -e "     ${YELLOW}FIX: lsof -ti:9090 | xargs kill -9${NC}"
    ERRORS=$((ERRORS + 1))
else
    echo -e "  ${GREEN}✓${NC} Port 9090 is available"
fi

echo ""
echo "========================================="
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✅ SETUP COMPLETE - NO ERRORS!${NC}"
    echo "========================================="
    echo ""
    echo "Next steps:"
    echo -e "  1. ${BLUE}./webserv webserv.conf${NC}     (start server)"
    echo -e "  2. ${BLUE}./evaluation_tester.py${NC}     (run tests)"
    echo ""
    echo "Expected: 18-22 tests passing (78-96%)"
else
    echo -e "${YELLOW}⚠ SETUP COMPLETE - $ERRORS WARNINGS${NC}"
    echo "========================================="
    echo ""
    echo -e "${YELLOW}Some issues were found. Please fix them before testing.${NC}"
    echo ""
fi

echo "Quick test commands:"
echo "  curl http://127.0.0.1:8080/"
echo "  curl http://127.0.0.1:8080/cgi/test.py"
echo "  curl http://127.0.0.1:8080/cgi-bin/session.py"
echo ""
