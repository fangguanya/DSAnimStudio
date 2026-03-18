import subprocess, sys

ue_cmd = r'D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
project = r'E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject'
export_dir = r'E:\Sekiro\Export'

cmd = [
    ue_cmd,
    project,
    '-run=SekiroImport',
    f'-ExportDir={export_dir}',
    '-ChrFilter=c0000',
    '-AnimLimit=10',
    '-unattended',
    '-nopause',
    '-nosplash',
    '-nullrhi',
]

print('Running:', ' '.join(f'"{a}"' if ' ' in a else a for a in cmd))
sys.stdout.flush()

result = subprocess.run(cmd, capture_output=False, text=True)
print(f'Exit code: {result.returncode}')
