#!/bin/bash
# Setup script - Run this after extracting the archive

echo "Setting up Webserv..."

# Make CGI scripts executable
echo "Making CGI scripts executable..."
chmod +x cgi-bin/*.py 2>/dev/null
chmod +x complete_test.py 2>/dev/null
chmod +x verify_setup.sh 2>/dev/null

# Create test files if missing
echo "Creating test files..."
mkdir -p www/delete_zone www/private_dir 2>/dev/null
echo "protected content" > www/delete_zone/protected.txt 2>/dev/null

# Verify
echo ""
echo "Verification:"
echo "- CGI scripts:"
ls -la cgi-bin/*.py | grep -E '^-rwx' && echo "  ✓ Executable" || echo "  ✗ Not executable - run: chmod +x cgi-bin/*.py"

echo "- Test files:"
[ -f www/delete_zone/protected.txt ] && echo "  ✓ protected.txt exists" || echo "  ✗ protected.txt missing"

echo ""
echo "Setup complete! Run:"
echo "  make"
echo "  ./webserv webserv.conf"
