# Webserv (Eval-Only Canonical Layout)

This repository is now organized around one deterministic evaluation workflow.

## What Is Canonical
- Source: `src/`, `include/`, `Makefile`
- Evaluation fixtures: `www_eval/`
- Canonical configs:
  - `config/eval_suite.conf`
  - `config/eval_secondary.conf`
  - `config/eval_port_conflict.conf`
- Canonical testers:
  - `./scripts/test_eval_quick.sh` (full suite)
  - `python3 scripts/test_eval_smoke.py` (fast smoke)
- Canonical docs:
  - `docs/TESTING.md`
  - `docs/MANUAL_TESTING.md`

## Build
```bash
make
```

## Run (Explicit Config Required)
```bash
./webserv config/eval_suite.conf
```

`./webserv` without a config argument is intentionally not part of the supported flow in this layout.

## Config Matrix
- `config/eval_suite.conf`
  - Primary evaluation profile.
  - Runs two servers: `127.0.0.1:18080` (site A) and `127.0.0.1:18081` (site B).
  - Covers routes for redirect, autoindex, upload/delete, max-body-size limits, CGI behavior, and error pages.
- `config/eval_secondary.conf`
  - Secondary standalone server on `127.0.0.1:18082` used for multi-instance checks.
- `config/eval_port_conflict.conf`
  - Intentionally conflicting listen config used to verify fast-fail behavior.

## Automated Testing
```bash
./scripts/test_eval_quick.sh
python3 scripts/test_eval_smoke.py
```

See [docs/TESTING.md](/home/zsalah/Downloads/webserv-main/docs/TESTING.md) for full details.

## Manual Testing (Without Testers)
See [docs/MANUAL_TESTING.md](/home/zsalah/Downloads/webserv-main/docs/MANUAL_TESTING.md).

## Safe Cleanup Workflow (Two-Step)
Step 1: Quarantine non-canonical files and generate a manifest.
```bash
./scripts/quarantine_legacy.sh
```

Step 2: Purge quarantined files only after validation passes.
```bash
./scripts/purge_quarantine.sh
```

## Notes
- The C++ server interfaces and CLI contract are unchanged.
- This cleanup intentionally favors evaluation determinism over legacy/demo profile compatibility.
