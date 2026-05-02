#!/usr/bin/python3
import os
import http.cookies
import time

print("Content-Type: text/html\r")

# Get cookies from request
cookie_string = os.environ.get('HTTP_COOKIE', '')
cookies = http.cookies.SimpleCookie()
if cookie_string:
    cookies.load(cookie_string)

# Get or create session ID
if 'session_id' in cookies:
    session_id = cookies['session_id'].value
    visit_count = int(cookies.get('visit_count', cookies.SimpleCookie({'visit_count': '0'}))['visit_count'].value) + 1
else:
    session_id = str(int(time.time()))
    visit_count = 1

# Set cookies
print(f"Set-Cookie: session_id={session_id}; Path=/; Max-Age=3600\r")
print(f"Set-Cookie: visit_count={visit_count}; Path=/; Max-Age=3600\r")
print(f"Set-Cookie: username=WebservUser; Path=/\r")
print("\r")

# HTML output
print(f"""
<html>
<head>
    <title>Session and Cookies Demo</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }}
        .container {{
            background: white;
            color: #333;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }}
        h1 {{ color: #667eea; }}
        .cookie-info {{
            background: #f0f0f0;
            padding: 15px;
            border-radius: 5px;
            margin: 15px 0;
        }}
        .success {{ color: #4caf50; font-weight: bold; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>✅ Session & Cookies Working!</h1>
        
        <div class="cookie-info">
            <h2>Your Session Information:</h2>
            <p><strong>Session ID:</strong> {session_id}</p>
            <p><strong>Visit Count:</strong> <span class="success">{visit_count}</span></p>
            <p><strong>Username Cookie:</strong> WebservUser</p>
        </div>
        
        <div class="cookie-info">
            <h2>How It Works:</h2>
            <ol>
                <li>Server sends <code>Set-Cookie</code> headers</li>
                <li>Browser stores cookies automatically</li>
                <li>Browser sends cookies back in <code>Cookie</code> header</li>
                <li>Server reads cookies from HTTP_COOKIE environment variable</li>
            </ol>
        </div>
        
        <p><strong>Reload this page</strong> to see the visit counter increase!</p>
        
        <p style="margin-top: 30px;">
            <a href="/cgi-bin/session.py" style="color: #667eea;">Refresh Page</a> | 
            <a href="/" style="color: #667eea;">Home</a>
        </p>
    </div>
</body>
</html>
""")
