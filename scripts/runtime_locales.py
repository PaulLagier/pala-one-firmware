Import("env")
from pathlib import Path
import subprocess
import sys

root = Path(env["PROJECT_DIR"])
languages = root / "Pala_One_2_1/src/lang"
subprocess.run([
    sys.executable, str(root / "scripts/generate_locales.py"),
    "--catalogs", str(languages / "catalogs"),
    "--out", str(languages),
    "--interface-source", str(languages.parent),
    "--require-all", "--quiet",
], check=True)
