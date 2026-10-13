"""Forward to the system-settings suite's create-only editor script."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).resolve().parents[1] /
    'ArtSource/FPSDemo/UI/SystemSettings/Scripts/build_system_settings_ui.py'),
    run_name='__main__')
