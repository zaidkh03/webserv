# Manual Testing (No Testers)

Start server:
```bash
make
./webserv config/eval_suite.conf
```

Use another terminal for the checks below.

## Basic Routing
```bash
curl -i http://127.0.0.1:18080/
curl -i http://127.0.0.1:18081/
curl -i http://127.0.0.1:18080/with-index/
```

Expected:
- `200 OK`
- Body markers include `EVAL SITE A ROOT`, `EVAL SITE B ROOT`, and `EVAL WITH INDEX PAGE`

## Redirect
```bash
curl -i http://127.0.0.1:18080/redirect-me
```

Expected:
- `301 Moved Permanently`
- `Location: /with-index/`

## Upload and Delete
```bash
curl -i -X POST -H "Content-Type: text/plain" --data "hello42" \
  http://127.0.0.1:18080/upload/manual.txt
curl -i http://127.0.0.1:18080/upload/manual.txt
curl -i -X DELETE http://127.0.0.1:18080/delete/manual.txt
curl -i http://127.0.0.1:18080/upload/manual.txt
```

Expected:
- POST returns `201`
- GET returns `200` with uploaded body
- DELETE returns `204`
- Final GET returns `404`

## Body Limit Enforcement
```bash
curl -i -X POST -H "Content-Type: text/plain" --data "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
  http://127.0.0.1:18080/tiny-body/too_big.txt
```

Expected:
- `413 Payload Too Large`

## Autoindex
```bash
curl -i http://127.0.0.1:18080/list/
curl -i http://127.0.0.1:18080/list
```

Expected:
- `200`
- Listing includes `autoindex_expected_a.txt` and `autoindex_expected_b.txt`

## HEAD Policy
```bash
curl -I http://127.0.0.1:18080/with-index/
curl -I http://127.0.0.1:18080/post-only
```

Expected:
- First request returns `200`
- Second request returns `405`

## CGI Env Echo
```bash
curl -i "http://127.0.0.1:18080/cgi-bin/env_echo.py?alpha=1&beta=two"
curl -i -X POST -H "Content-Type: text/plain" --data "payload123" \
  http://127.0.0.1:18080/cgi-bin/env_echo.py
```

Expected:
- `200`
- JSON fields reflect method, query string, URI, and body metadata

## Malformed Request Resilience
```bash
printf 'GETTTTT / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n' | nc 127.0.0.1 18080
curl -i http://127.0.0.1:18080/with-index/
```

Expected:
- First command returns an HTTP error response (`4xx` or `5xx`)
- Server remains responsive (`200` on second command)
