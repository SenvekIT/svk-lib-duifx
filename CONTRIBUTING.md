# Contributing to DUIFx

Thanks for your interest in improving DUIFx. This document covers how to get a
development environment running and what we expect from a change.

## Development setup

Python 3.10 or newer is required.

```bash
git clone https://github.com/SenvekIT/svk-lib-duifx.git
cd svk-lib-duifx
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements-dev.txt
pip install -e .
```

## Working on the code

### Qt resources

Icons and stylesheets are compiled into `duifx/gui/ui/resource_rc.py` through the
Qt resource system. After adding, removing or renaming anything under
`duifx/gui/resources/`, update `resource.qrc` and recompile:

```bash
pyside6-rcc -o duifx/gui/ui/resource_rc.py duifx/gui/resources/resource.qrc
```

Commit both the changed resource files and the regenerated `resource_rc.py`,
otherwise the two drift apart.

> On Windows, git does not notice a rename that only changes capitalisation
> (`core.ignorecase`). If you rename `Foo.svg` to `foo.svg`, verify the index
> with `git ls-files` — a stale entry breaks the build on case-sensitive
> systems such as Linux CI.

### Icons

Bundled icons must come from a source whose licence permits redistribution.
The current set is [Flat Color Icons](https://github.com/icons8/flat-color-icons)
(MIT). Do not add icons from sources that forbid redistributing the files, and
do not add third-party brand or product logos — the artwork may be licensed
while the trademark it depicts is not. Record the origin of anything new in
[THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md).

### Building

```bash
python -m build
```

## Pull requests

- Keep the change focused; unrelated cleanups belong in their own commit.
- Match the surrounding code style rather than reformatting existing code.
- Describe what the change does and why, not just which files it touches.
- If the change affects behaviour visible to users of the library, add an entry
  to [CHANGELOG.md](CHANGELOG.md) under "Unreleased".

## Licence of contributions

DUIFx is released under the MIT Licence. By contributing you agree that your
contribution is licensed under the same terms.
