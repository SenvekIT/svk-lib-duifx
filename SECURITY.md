# Security Policy

## Supported versions

Security fixes are applied to the latest released version. Older versions are
not maintained.

| Version | Supported |
|---------|-----------|
| 1.3.x   | ✅        |
| < 1.3   | ❌        |

## Reporting a vulnerability

Please report security issues **privately** — do not open a public GitHub issue.

- Email: **info@senvek.com** with `DUIFx security` in the subject, or
- use GitHub's [private vulnerability reporting](https://github.com/SenvekIT/svk-lib-duifx/security/advisories/new).

Helpful details: affected version, platform and Python version, what an attacker
could achieve, and a minimal reproduction if you have one.

We aim to acknowledge a report within five working days and will keep you
informed while we investigate. Please give us reasonable time to release a fix
before disclosing the issue publicly.

## Scope

DUIFx is a desktop UI library. It does not process untrusted network input on
its own, so the realistic attack surface is small — most relevant are issues in
how applications feed untrusted data into DUIFx widgets (for example log
content, file paths or stylesheet fragments).

Vulnerabilities in [Qt for Python (PySide6)](https://doc.qt.io/qtforpython-6/)
itself belong to The Qt Company and should be reported through their channels.
