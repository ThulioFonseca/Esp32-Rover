"""
build_web.py — Wrapper SCons para PlatformIO extra_scripts.
Invoca build_web_impl.py como pre-script antes de qualquer compilação.

Como funciona:
  - O extra_scripts = pre:tools/build_web.py garante que este script roda
    no início de todo `pio run`, antes que qualquer .cpp seja compilado.
  - build_web_impl.py só reescreve web_pages.h se o conteúdo mudou.
  - Se web_pages.h mudou, o SCons detecta e recompila web_server_manager.cpp.
  - Se web_pages.h não mudou, o SCons pula a recompilação (build incremental).

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

print("[build_web] Rodando pipeline de build dos assets web...")
result = subprocess.run(
    [sys.executable, str(_impl)],
    cwd=str(_project_dir)
)
if result.returncode != 0:
    print("[build_web] ERRO: falha no build dos assets web!")
    env.Exit(1)
