"""Stable Unreal Python entry point; the asset-owned script is authoritative."""
from pathlib import Path
import runpy

script = (Path(__file__).resolve().parents[1] /
          'ArtSource/FPSDemo/UI/ExtractionTerminal/Scripts/build_extraction_ui.py')
runpy.run_path(str(script), run_name='__main__')
