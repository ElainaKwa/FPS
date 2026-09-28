"""Run via UnrealEditor-Cmd -run=pythonscript -script=<this file>."""
from pathlib import Path
import unreal

source = Path(__file__).resolve().parents[1] / 'ArtSource/FPSDemo/Audio'
tasks=[]
for path in sorted(source.glob('*.wav')):
    task=unreal.AssetImportTask()
    task.filename=str(path)
    task.destination_path='/Game/FPSDemo/Audio'
    task.destination_name=path.stem
    task.automated=True
    task.replace_existing=True
    task.save=True
    task.factory=unreal.SoundFactory()
    tasks.append(task)
assert len(tasks)==13, 'Expected 13 authored sounds'
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    asset=unreal.load_asset('/Game/FPSDemo/Audio/'+task.destination_name)
    assert isinstance(asset,unreal.SoundWave), task.destination_name
    duration=asset.get_editor_property('duration')
    assert duration>0, task.destination_name
    unreal.log('FPS_AUDIO_IMPORT PASS '+task.destination_name+' duration='+str(duration))
unreal.log('FPS_AUDIO_IMPORT COMPLETE 13')
