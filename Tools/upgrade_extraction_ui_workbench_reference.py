"""Forward to the suite's explicit v3 -> v4 reference-workbench migration."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).resolve().parents[1] /
    'ArtSource/FPSDemo/UI/ExtractionTerminal/Scripts/upgrade_extraction_ui_workbench_reference.py'),
    run_name='__main__')
