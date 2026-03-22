$ErrorActionPreference = 'Stop'

$ue = 'D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe'
$project = 'E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject'
$script = 'E:\Sekiro\DSAnimStudio\tools\build_sekiro_to_manny_retarget_assets.py'

$cmd = @(
    $project,
    "-ExecutePythonScript=$script",
    '-nopause',
    '-nosplash'
)

Write-Host 'Running formal Sekiro retarget builder and animation retarget batch...'
Write-Host $ue
Write-Host ($cmd -join ' ')

$process = Start-Process -FilePath $ue -ArgumentList $cmd -Wait -PassThru
exit $process.ExitCode