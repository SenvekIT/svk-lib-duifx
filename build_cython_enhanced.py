"""
Enhanced Cython build script with better obfuscation.
filepath: build_cython_enhanced.py
"""

import os
import re
import sys
import shutil
from pathlib import Path
from setuptools import setup, Extension, find_packages
from Cython.Build import cythonize

def get_python_files_to_compile():
    """Sammelt alle Python-Dateien für Kompilierung"""
    files_to_compile = []
    exclude_patterns = [
        "__init__.py",  # Nicht kompilieren
        "build_*.py",
        "test_*.py",
    ]
    
    for root, dirs, files in os.walk("duifx"):
        # Ignoriere bestimmte Verzeichnisse
        dirs[:] = [d for d in dirs if d not in ['__pycache__', '.git', 'tests', 'examples']]
        
        for file in files:
            if file.endswith('.py'):
                # Prüfe ob Datei ausgeschlossen werden soll
                if any(pattern in file for pattern in exclude_patterns):
                    continue
                
                filepath = os.path.join(root, file)
                files_to_compile.append(filepath)
    
    return files_to_compile

def get_extensions():
    """Erstellt Extensions für alle Python-Dateien"""
    py_files = get_python_files_to_compile()
    extensions = []
    
    for py_file in py_files:
        module_name = py_file.replace(os.sep, '.').replace('.py', '')
        
        extensions.append(
            Extension(
                module_name,
                [py_file],
                extra_compile_args=['/O2', '/GL'] if sys.platform == 'win32' else ['-O3', '-march=native'],
                extra_link_args=['/LTCG'] if sys.platform == 'win32' else [],
                define_macros=[('CYTHON_TRACE', '0')],  # Deaktiviere Tracing
            )
        )
    
    return extensions

def read_package_version(path: Path, key: str) -> str:
    """
    Liest `__version__` aus `__init__.py` ohne Import.
    """
    try:
        text = path.read_text(encoding="utf-8")
    except FileNotFoundError:
        return "0.0.0"

    escaped_key = re.escape(key)
    pattern = r'^' + escaped_key + r'\s*=\s*([\'"])(?P<v>.+?)\1'
    m = re.search(pattern, text, re.M)
    return m.group("v") if m else "0.0.0"

def build_cython_package():
    """Kompiliert das Package mit Cython"""
    
    print("="*60)
    print("Enhanced Cython Build for DUIFx")
    print("="*60)
    
    # Cleanup
    print("\nCleaning up old builds...")
    for directory in ['build', 'dist', 'duifx.egg-info']:
        if os.path.exists(directory):
            shutil.rmtree(directory)
            print(f"  Removed: {directory}")
    
    # Sammle Dateien
    print("\nCollecting Python files for compilation...")
    py_files = get_python_files_to_compile()
    print(f"  Found {len(py_files)} files to compile")
    
    for py_file in py_files:
        module_name = py_file.replace(os.sep, '.').replace('.py', '')
        print(f"  - {module_name}")
    
    # Erstelle Extensions
    print("\nCreating Cython extensions...")
    extensions = get_extensions()
    
    # Cythonize mit erweiterten Optionen
    print("\nCompiling with Cython...")
    ext_modules = cythonize(
        extensions,
        compiler_directives={
            'language_level': "3",
            'embedsignature': False,      # Keine Signaturen einbetten
            'boundscheck': False,          # Performance
            'wraparound': False,           # Performance
            'cdivision': True,             # Performance
            'always_allow_keywords': False, # Performance
            'profile': False,              # Kein Profiling
            'linetrace': False,            # Kein Line-Tracing
            'c_string_type': 'unicode',
            'c_string_encoding': 'utf-8',
        },
        nthreads=os.cpu_count(),
        force=True,
        annotate=False,  # Keine HTML-Annotationen
    )
    
    # Read version
    init_path = Path("duifx") / "version.py"
    version = {
        "__version__": read_package_version(init_path, "__version__"),
        "__author__": read_package_version(init_path, "__author__"),
        "__email__": read_package_version(init_path, "__email__"),
    }

    # Build mit setup()
    print("\nBuilding wheel package...")
    setup(
        name='duifx',
        version=version.get("__version__", "0"),
        author=version.get("__author__", ""),
        url="https://github.com/davidsenoner/duifx",
        author_email=version.get("__email__", ""),
        description="PySide6-based GUI framework",
        # packages=['duifx', 'duifx.gui','duifx.gui.resources', 'duifx.gui.ui', 'duifx.gui.windows', 'duifx.gui.settings', 'duifx.gui.utils'],
        packages=find_packages(exclude=["tests", "tests.*", "examples", "examples.*"]),
        ext_modules=ext_modules,
        package_data={
            'duifx': [
                'gui/resources/*.qss',
                'gui/resources/icon/*.svg',
                #'gui/ui/*.ui',
                '**/*.pyd',
                '**/*.so',
            ],
        },
        include_package_data=True,
        zip_safe=False,
        python_requires=">=3.8",
        script_name='build_cython_enhanced.py',
        script_args=['build_ext', 'bdist_wheel'],
    )
    
    print("\n" + "="*60)
    print("✅ Build completed successfully!")
    print("="*60)
    
    # Zeige erstellte Dateien
    print("\nCompiled extensions:")
    for ext in ['pyd', 'so']:
        for file in Path('.').rglob(f'*.{ext}'):
            print(f"  {file}")
    
    print("\nWheel packages:")
    if Path('dist').exists():
        for file in Path('dist').rglob('*.whl'):
            print(f"  {file}")
            print(f"\nTo install: pip install {file}")
    
    # Cleanup C-Dateien
    print("\nCleaning up C files...")
    for c_file in Path('.').rglob('*.c'):
        if 'duifx' in str(c_file):
            c_file.unlink()
            print(f"  Removed: {c_file}")

if __name__ == "__main__":
    try:
        build_cython_package()
    except Exception as e:
        print(f"\n❌ Build failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)