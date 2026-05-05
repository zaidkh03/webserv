# Testing Guide

## 1. Static Sanity Checks
```bash
bash -n scripts/test_eval_quick.sh scripts/quarantine_legacy.sh scripts/purge_quarantine.sh
python3 -m py_compile scripts/test_eval_smoke.py
```

## 2. Build
```bash
make
```

## 3. Full Evaluation Suite
```bash
./scripts/test_eval_quick.sh
```

Expected result:
- Exit code `0`
- Final summary shows `RESULT: PASS`

What it validates:
- Core routes and content markers
- Redirect semantics
- Upload/delete and body-size enforcement
- Autoindex behavior
- HEAD policy
- CGI env/body/PATH_INFO behavior
- Malformed HTTP resilience
- Non-blocking behavior under slow/partial clients
- Idle timeout behavior
- Port conflict and multi-instance behavior

## 4. Smoke Suite (Fast)
```bash
python3 scripts/test_eval_smoke.py
```

Expected result:
- Exit code `0`
- `PASS` lines for all smoke checks and final `Smoke suite passed`

## 5. Manual Checks
Use [MANUAL_TESTING.md](/home/zsalah/Downloads/webserv-main/docs/MANUAL_TESTING.md) when you want to verify behavior without any tester scripts.
