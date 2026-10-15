"""Project entry point for the suite-owned factory map generator."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).resolve().parents[1]/'ArtSource/FPSDemo/Environment/FactoryBase/Scripts/create_factory_base.py'),run_name='__main__')
