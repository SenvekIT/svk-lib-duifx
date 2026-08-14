# Third-Party Licenses

DUIFx itself is released under the MIT License (see [LICENSE](LICENSE)).
It bundles or depends on the third-party material listed below.

---

## Flat Color Icons — bundled

All icons under `duifx/gui/resources/icon/` that are not listed as
"in-house" below come from the **Flat Color Icons** set by Icons8.

- Project: <https://github.com/icons8/flat-color-icons>
- License: MIT, or alternatively the
  [Good Boy License](https://icons8.com/good-boy-license/) — the icon pack is
  released under either. DUIFx redistributes them under the **MIT** option.
- Redistribution and modification are permitted; no attribution is required by
  the license, this notice is provided as good practice.

```
MIT License

Copyright (c) Icons8 LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### In-house icons

The following 16 icons are original work by Senvek SRL, not part of the Flat
Color Icons set. They are covered by the DUIFx license (see [LICENSE](LICENSE)):

`Alert.svg`, `arrow-down-icon.svg`, `arrow-up-icon.svg`, `Channels.svg`,
`Dx.svg`, `File Error.svg`, `Master.svg`, `minus_bw.svg`,
`object_directory.svg`, `plus_bw.svg`, `qr.svg`, `record.svg`, `Risk.svg`,
`Slave.svg`, `Uncheck.svg`, `Uncheck 1.svg`

`Dx.svg` is the DUIFx logo. Note that the MIT license covers its use as
software; it does not grant permission to use the mark to represent another
product or organisation.

---

## Qt for Python (PySide6) — dependency, not bundled

- Project: <https://doc.qt.io/qtforpython-6/>
- License: **LGPLv3** (Qt is also available under GPL and under a commercial
  license from The Qt Company).

DUIFx imports PySide6 as a separate, dynamically linked package and does not
modify it. Users can replace or upgrade PySide6 independently through their
Python package manager, as required by the LGPL.

Applications distributing DUIFx together with PySide6 — in particular when
freezing everything into a single executable (PyInstaller one-file and
similar) — must satisfy the LGPLv3 obligations themselves, including making
the Qt source available and allowing the Qt libraries to be replaced.

The full license texts are available at:

- LGPLv3: <https://www.gnu.org/licenses/lgpl-3.0.html>
- Qt licensing overview: <https://www.qt.io/licensing/>
