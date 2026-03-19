"""
build_web.py — Wrapper SCons para PlatformIO extra_scripts.
Invoca build_web_impl.py como pre-action antes de compilar web_server_manager.cpp.

Configurar em platformio.ini:
    extra_scripts = pre:tools/build_web.py
"""

import subprocess
import sys
import os
from pathlib import Path

Import("env")  # noqa: F821 — SCons context

# Em contexto SCons, __file__ não está disponível — usa PROJECT_DIR do env
_project_dir = Path(env.get("PROJECT_DIR", os.getcwd()))
_impl = _project_dir / "tools" / "build_web_impl.py"


def _run_build_web(source, target, env):
    print("[build_web] Rodando pipeline de build dos assets web...")
    result = subprocess.run(
        [sys.executable, str(_impl)],
        cwd=str(_project_dir)
    )
    if result.returncode != 0:
        print("[build_web] ERRO: falha no build dos assets web!")
        env.Exit(1)


# Executa antes de compilar o arquivo que inclui web_pages.h
env.AddPreAction(
    "$BUILD_DIR/src/web/web_server_manager.cpp.o",
    _run_build_web
)
