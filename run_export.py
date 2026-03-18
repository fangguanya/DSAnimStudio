import shutil, os, subprocess, sys

export_dir = r'E:\Sekiro\Export\c0000'
if os.path.exists(export_dir):
    shutil.rmtree(export_dir)
    print(f'Cleared: {export_dir}')
else:
    print(f'Not found (ok): {export_dir}')

exporter = r'E:\Sekiro\DSAnimStudio\SekiroExporter\bin\Release\net6.0-windows\SekiroExporter.exe'
game_dir = r'C:\Program Files (x86)\Steam\steamapps\common\Sekiro'
output_dir = r'E:\Sekiro\Export'

cmd = [
    exporter,
    'export-all',
    '--game-dir', game_dir,
    '--chr', 'c0000',
    '--output', output_dir,
]

print('Running:', ' '.join(f'"{a}"' if ' ' in a else a for a in cmd))
sys.stdout.flush()

result = subprocess.run(cmd, capture_output=False, text=True)
print(f'Exit code: {result.returncode}')
