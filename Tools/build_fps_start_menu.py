"""Compatibility entry point for the start screen authoring script."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).resolve().parents[1] / 'ArtSource/FPSDemo/UI/StartMenu/Scripts/build_start_menu.py'), run_name='__main__')
