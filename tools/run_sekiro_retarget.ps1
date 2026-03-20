$ErrorActionPreference = 'Stop'

$ue = 'D:\UE_OFFICIAL\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project = 'E:\Sekiro\DSAnimStudio\SekiroSkillEditor\SekiroSkillEditor.uproject'
$sourceMesh = '/Game/SekiroAssets/Characters/c0000/Mesh/c0000/SkeletalMeshes/c0000.c0000'
$outputBase = '/Game/Retargeting/SekiroToManny'
$animationBase = '/Game/SekiroAssets/Characters/c0000/Animations'

$clips = @(
    'a000_201030',
    'a000_201050',
    'a000_202010',
    'a000_202011',
    'a000_202012',
    'a000_202035',
    'a000_202100',
    'a000_202110',
    'a000_202112',
    'a000_202300',
    'a000_202310',
    'a000_202400',
    'a000_202410',
    'a000_202600',
    'a000_202610',
    'a000_202700',
    'a000_202710'
)

$animations = ($clips | ForEach-Object { "$animationBase/$_.$_" }) -join ','

$cmd = @(
    $project,
    '-run=SekiroRetarget',
    "-SourceMesh=$sourceMesh",
    "-Animations=$animations",
    "-OutputBase=$outputBase",
    '-unattended',
    '-nopause',
    '-nosplash',
    '-nullrhi'
)

Write-Host 'Running formal Sekiro retarget commandlet...'
Write-Host $ue
Write-Host ($cmd -join ' ')

& $ue @cmd
exit $LASTEXITCODE