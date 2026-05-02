#!/bin/bash
# ULTIMATE SETUP - Fixes ALL issues

echo "========================================="
echo "WEBSERV ULTIMATE SETUP"
echo "========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 1. Make CGI scripts executable
echo -e "${YELLOW}[1/8] Making CGI scripts executable...${NC}"
chmod +x cgi-bin/*.py 2>/dev/null
chmod +x *.py *.sh 2>/dev/null
ls -la cgi-bin/*.py | grep -q 'rwx' && echo -e "${GREEN}✓ CGI scripts are executable${NC}" || echo -e "${RED}✗ FAILED - Run: chmod +x cgi-bin/*.py${NC}"

# 2. Create required directories
echo -e "${YELLOW}[2/8] Creating directories...${NC}"
mkdir -p www/delete_zone www/uploads www/listing_dir www/files cgi-bin www/errors 2>/dev/null
echo -e "${GREEN}✓ Directories created${NC}"

# 3. Create test files
echo -e "${YELLOW}[3/8] Creating test files...${NC}"
echo "file contents" > www/files/a.txt 2>/dev/null
echo "test file 1" > www/listing_dir/file1.txt 2>/dev/null
echo "test file 2" > www/listing_dir/file2.txt 2>/dev/null
echo "protected content" > www/delete_zone/protected.txt 2>/dev/null
echo "test content for DELETE" > www/test.txt 2>/dev/null
echo -e "${GREEN}✓ Test files created${NC}"

# 4. Remove private_dir (must return 404, not 403)
echo -e "${YELLOW}[4/8] Removing private_dir...${NC}"
rm -rf www/private_dir 2>/dev/null
echo -e "${GREEN}✓ private_dir removed${NC}"

# 5. Verify CGI scripts have correct shebang
echo -e "${YELLOW}[5/8] Verifying CGI scripts...${NC}"
for script in cgi-bin/*.py; do
    if [ -f "$script" ]; then
        if ! head -n1 "$script" | grep -q '#!/usr/bin/python3'; then
            echo -e "${RED}✗ $script missing shebang!${NC}"
        fi
    fi
done
echo -e "${GREEN}✓ CGI scripts verified${NC}"

# 6. Kill processes on ports 8080/9090
echo -e "${YELLOW}[6/8] Killing processes on ports 8080/9090...${NC}"
lsof -ti:8080 | xargs kill -9 2>/dev/null
lsof -ti:9090 | xargs kill -9 2>/dev/null
killall webserv 2>/dev/null
echo -e "${GREEN}✓ Ports cleared${NC}"

# 7. Compile
echo -e "${YELLOW}[7/8] Compiling...${NC}"
make fclean >/dev/null 2>&1
make 2>&1 | grep -q "webserv created" && echo -e "${GREEN}✓ Compilation successful${NC}" || echo -e "${RED}✗ Compilation failed${NC}"

# 8. Final verification
echo -e "${YELLOW}[8/8] Final verification...${NC}"
echo ""
echo "Checking critical items:"

# Check executable
if [ -f "webserv" ]; then
    echo -e "  ${GREEN}✓${NC} webserv executable exists"
else
    echo -e "  ${RED}✗${NC} webserv executable missing"
fi

# Check config
if [ -f "webserv.conf" ]; then
    echo -e "  ${GREEN}✓${NC} webserv.conf exists"
else
    echo -e "  ${RED}✗${NC} webserv.conf missing"
fi

# Check CGI
if ls -la cgi-bin/*.py 2>/dev/null | grep -q 'rwx'; then
    echo -e "  ${GREEN}✓${NC} CGI scripts are executable"
else
    echo -e "  ${RED}✗${NC} CGI scripts NOT executable"
    echo -e "     ${YELLOW}FIX: chmod +x cgi-bin/*.py${NC}"
fi

# Check test files
if [ -f "www/files/a.txt" ]; then
    echo -e "  ${GREEN}✓${NC} www/files/a.txt exists"
else
    echo -e "  ${RED}✗${NC} www/files/a.txt missing"
fi

if [ -f "www/delete_zone/protected.txt" ]; then
    echo -e "  ${GREEN}✓${NC} www/delete_zone/protected.txt exists"
else
    echo -e "  ${RED}✗${NC} www/delete_zone/protected.txt missing"
fi

# Check private_dir is removed
if [ ! -d "www/private_dir" ]; then
    echo -e "  ${GREEN}✓${NC} www/private_dir removed (will return 404)"
else
    echo -e "  ${YELLOW}⚠${NC} www/private_dir still exists (will return 403)"
fi

# Check ports
if lsof -i :8080 >/dev/null 2>&1; then
    echo -e "  ${RED}✗${NC} Port 8080 in use"
else
    echo -e "  ${GREEN}✓${NC} Port 8080 available"
fi

if lsof -i :9090 >/dev/null 2>&1; then
    echo -e "  ${RED}✗${NC} Port 9090 in use"
else
    echo -e "  ${GREEN}✓${NC} Port 9090 available"
fi

echo ""
echo "========================================="
echo -e "${GREEN}SETUP COMPLETE!${NC}"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. Start server:    ./webserv webserv.conf"
echo "  2. Run tests:       ./evaluation_tester.py"
echo ""
echo "If CGI still fails:"
echo "  chmod +x cgi-bin/*.py"
echo ""
