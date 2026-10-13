"""Forward to the suite-owned additive v2 -> v3 weapon workbench migration."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).resolve().parents[1] /
    'ArtSource/FPSDemo/UI/ExtractionTerminal/Scripts/upgrade_extraction_ui_workbench.py'),
    run_name='__main__')
