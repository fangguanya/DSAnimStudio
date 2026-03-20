$ErrorActionPreference = 'Stop'

$ue = 'D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project = 'E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject'
$exportDir = 'E:\Sekiro\Export'
$contentBase = '/Game/SekiroAssets/Characters'

$cmd = @(
    $project,
    '-run=SekiroImport',
    "-ExportDir=$exportDir",
    '-ChrFilter=c0000',
    "-ContentBase=$contentBase",
    '-ImportAnimationsOnly=true',
    '-unattended',
    '-nopause',
    '-nosplash',
    '-nullrhi'
)

Write-Host 'Running formal Sekiro animation-only import against the current export directory...'
Write-Host $ue
Write-Host ($cmd -join ' ')

& $ue @cmd
exit $LASTEXITCODE