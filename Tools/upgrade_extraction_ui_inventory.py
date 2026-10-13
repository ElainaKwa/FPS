"""Forward to the suite-owned v2 factory/grid UI migration."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).resolve().parents[1] /
    'ArtSource/FPSDemo/UI/ExtractionTerminal/Scripts/upgrade_extraction_ui_inventory.py'),
    run_name='__main__')
