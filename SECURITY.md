# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in Cooledit, please report it
privately by email to paulsheer@gmail.com.  Do **not** file a public
issue.

We aim to acknowledge your report within 7 days and will keep you
informed of our progress.  We request that you allow a reasonable
window for a fix to be released before disclosing details publicly.

We appreciate responsible disclosure and will not take legal action
against good-faith reporters.

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| latest  | :white_check_mark: |
| < latest | :x:                |

## Scope

Issues that may qualify:

- Arbitrary code execution through crafted input files
- Buffer overflows, use-after-free, or other memory-safety bugs
- Privilege escalation via the editor's file-handling or IPC

Issues that are **not** considered vulnerabilities:

- The editor running user-supplied scripts or build commands (this is
  by design)
- Denial of service through resource exhaustion on local input
