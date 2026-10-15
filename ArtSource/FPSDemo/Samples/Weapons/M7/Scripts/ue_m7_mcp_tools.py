"""Register this asset library's fixed import/check operations with UE MCP.

Launch the editor with -ExecutePythonScript=THIS_FILE. Tools accept no paths or
arbitrary code and can only operate on this M7 art library. This does not run
the import when registering the tools.
"""
import json
import runpy
from pathlib import Path

import unreal
import toolset_registry

LIBRARY = Path(__file__).resolve().parents[1]
PROJECT = Path(unreal.Paths.project_dir()).resolve()
PREVIEW_NAMESPACE = None


@unreal.uclass()
class M7ArtLibraryTools(unreal.ToolsetDefinition):
    """Import and verify the project's existing categorized M7 art library."""

    @toolset_registry.tool_call
    @staticmethod
    def import_library() -> str:
        """Import the fixed validated M7 FBXs/maps, assign materials and save.

        Imports 19 parts plus three merged visual presets to the categorized
        Weapons folders. Refuses to overwrite unrelated assets.
        """
        runpy.run_path(str(LIBRARY / 'Scripts/ue_import_m7.py'), run_name='__main__')
        report = json.loads((LIBRARY / 'UnrealImportReport.json').read_text(encoding='utf-8'))
        return json.dumps({'success': report['success'], 'summary': report.get('summary'),
                           'errors': report['errors']})

    @toolset_registry.tool_call
    @staticmethod
    def verify_library() -> str:
        """Check saved M7 meshes, material references, maps, scale and sockets."""
        scope = runpy.run_path(str(LIBRARY / 'Scripts/ue_verify_m7.py'), run_name='m7_verification')
        scope['main'](require_fresh_process=False)
        report = json.loads((LIBRARY / 'UnrealVerification.json').read_text(encoding='utf-8'))
        return json.dumps({'success': report['success'], 'summary': report.get('summary'),
                           'errors': report.get('errors'),
                           'fresh_process_since_import_report_write': report.get('process_evidence', {}).get('fresh_process_since_import_report_write')})

    @toolset_registry.tool_call
    @staticmethod
    def build_preview() -> str:
        """Build the dedicated M7 preview level in this task's editor session.

        Requires a complete import. Does not replace gameplay levels.
        """
        global PREVIEW_NAMESPACE
        PREVIEW_NAMESPACE = runpy.run_path(str(LIBRARY / 'Scripts/ue_preview_m7.py'), run_name='m7_preview')
        cameras = PREVIEW_NAMESPACE['build_preview']()
        return json.dumps({'map': PREVIEW_NAMESPACE['MAP'], 'cameras': [x[0] for x in cameras]})

    @toolset_registry.tool_call
    @staticmethod
    def capture_preview() -> str:
        """Capture the dedicated preview using the real Unreal renderer.

        Requires a renderer (not -nullrhi). Produces only task evidence PNGs.
        """
        if '-nullrhi' in unreal.SystemLibrary.get_command_line().lower():
            raise RuntimeError('Screenshots require an Unreal editor with rendering enabled')
        if PREVIEW_NAMESPACE is None:
            raise RuntimeError('Build the dedicated M7 preview first')
        PREVIEW_NAMESPACE['begin_capture'](quit_after=False)
        return 'Capture scheduled; results: Saved/M7UnrealImport/Previews/CaptureReport.json'


unreal.ToolsetRegistry.register_toolset_class(M7ArtLibraryTools)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.log('M7_MCP_TOOLS_READY')
