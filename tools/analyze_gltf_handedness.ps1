param(
    [string]$GltfPath = "E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\Export_17_Preview\Model\c0000.gltf"
)

$ErrorActionPreference = 'Stop'

$gltf = Get-Content $GltfPath -Raw | ConvertFrom-Json
$nodes = $gltf.nodes

$parentByNode = @{}
for ($i = 0; $i -lt $nodes.Count; $i++) {
    if ($nodes[$i].children) {
        foreach ($child in $nodes[$i].children) {
            $parentByNode[[int]$child] = $i
        }
    }
}

$indexByName = @{}
for ($i = 0; $i -lt $nodes.Count; $i++) {
    if ($nodes[$i].name) {
        $indexByName[$nodes[$i].name] = $i
    }
}

function New-IdentityMatrix {
    $m = New-Object 'double[,]' 4,4
    $m[0,0] = 1; $m[1,1] = 1; $m[2,2] = 1; $m[3,3] = 1
    return $m
}

function Multiply-Matrix($a, $b) {
    $m = New-Object 'double[,]' 4,4
    for ($r = 0; $r -lt 4; $r++) {
        for ($c = 0; $c -lt 4; $c++) {
            $sum = 0.0
            for ($k = 0; $k -lt 4; $k++) {
                $sum += $a[$r, $k] * $b[$k, $c]
            }
            $m[$r, $c] = $sum
        }
    }
    return $m
}

function Get-LocalMatrix($node) {
    $t = if ($node.translation) { $node.translation } else { @(0.0, 0.0, 0.0) }
    $r = if ($node.rotation) { $node.rotation } else { @(0.0, 0.0, 0.0, 1.0) }
    $s = if ($node.scale) { $node.scale } else { @(1.0, 1.0, 1.0) }

    $qx = [double]$r[0]
    $qy = [double]$r[1]
    $qz = [double]$r[2]
    $qw = [double]$r[3]

    $xx = $qx * $qx
    $yy = $qy * $qy
    $zz = $qz * $qz
    $xy = $qx * $qy
    $xz = $qx * $qz
    $yz = $qy * $qz
    $wx = $qw * $qx
    $wy = $qw * $qy
    $wz = $qw * $qz

    $m = New-IdentityMatrix
    $m[0,0] = (1 - 2 * ($yy + $zz)) * [double]$s[0]
    $m[0,1] = (2 * ($xy - $wz)) * [double]$s[1]
    $m[0,2] = (2 * ($xz + $wy)) * [double]$s[2]
    $m[0,3] = [double]$t[0]

    $m[1,0] = (2 * ($xy + $wz)) * [double]$s[0]
    $m[1,1] = (1 - 2 * ($xx + $zz)) * [double]$s[1]
    $m[1,2] = (2 * ($yz - $wx)) * [double]$s[2]
    $m[1,3] = [double]$t[1]

    $m[2,0] = (2 * ($xz - $wy)) * [double]$s[0]
    $m[2,1] = (2 * ($yz + $wx)) * [double]$s[1]
    $m[2,2] = (1 - 2 * ($xx + $yy)) * [double]$s[2]
    $m[2,3] = [double]$t[2]
    return $m
}

$worldCache = @{}
function Get-WorldMatrix([int]$nodeIndex) {
    if ($worldCache.ContainsKey($nodeIndex)) {
        return $worldCache[$nodeIndex]
    }

    $local = Get-LocalMatrix $nodes[$nodeIndex]
    if ($parentByNode.ContainsKey($nodeIndex)) {
        $world = Multiply-Matrix (Get-WorldMatrix $parentByNode[$nodeIndex]) $local
    } else {
        $world = $local
    }

    $worldCache[$nodeIndex] = $world
    return $world
}

function Get-WorldLocation([string]$boneName) {
    if (-not $indexByName.ContainsKey($boneName)) {
        return $null
    }

    $w = Get-WorldMatrix ([int]$indexByName[$boneName])
    $x = $w.GetValue(0, 3)
    $y = $w.GetValue(1, 3)
    $z = $w.GetValue(2, 3)
    return [PSCustomObject]@{
        bone = $boneName
        x = [math]::Round($x, 3)
        y = [math]::Round($y, 3)
        z = [math]::Round($z, 3)
    }
}

$bones = 'Master', 'Pelvis', 'Head', 'L_Shoulder', 'R_Shoulder', 'L_Hand', 'R_Hand', 'L_Foot', 'R_Foot'
$bones | ForEach-Object { Get-WorldLocation $_ } | Format-Table -AutoSize

$left = Get-WorldLocation 'L_Hand'
$right = Get-WorldLocation 'R_Hand'
if ($left -and $right) {
    Write-Host "source_lateral_delta=($([math]::Round($left.x - $right.x, 3)), $([math]::Round($left.y - $right.y, 3)), $([math]::Round($left.z - $right.z, 3)))"
}