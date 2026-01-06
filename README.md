# Introduction 
**DUIFx** is python PySide6 based user interface package that provides windows, dialogs and widgets with predefined styles.

## Consuming package
1. Install requirements:
    ```console
    pip install keyring artifacts-keyring
    ```
2. Install the package using pip:
    ```console
    pip install --index-url=https://pkgs.dev.azure.com/davtecIT/_packaging/davtecIT/pypi/simple/ duifx
    ```
2. When installing from requirements file be sure you have the extra index url defined:
    ```
    --extra-index-url https://pkgs.dev.azure.com/davtecIT/_packaging/davtecIT/pypi/simple/
    ``` 
## Build & Release

1. Install dependencies:
```
pip install -r requirements-dev.txt
```

2. Compile Qt resource file:
```
pyside6-rcc -o resources_rc.py resources.qrc
```

3. Build package:
```
python build_cython_enhanced.py
```

4. Upload package to Devops Artifacts:
```
$env:TWINE_USERNAME="__token__"
$env:TWINE_PASSWORD="pypi-REPLACE_WITH_TOKEN"

python -m twine upload -r davtecIT --verbose --config-file .pypirc dist/*
```
