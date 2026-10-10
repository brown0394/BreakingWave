# Generates Tools/TrenchCenterline.json, the one file both trench consumers read: GenerateBeachHeightmap.ps1
# carves the channel from it, and workstream C's placement script emits parapet, firing steps and TrenchNode
# actors from it (Decisions 052, 056, 057).
#
# The authored part is the Route table below: a straight-edged route whose labelled points are fixed (line
# ends, junctions, bunker bypass corners, doors, outposts, bluff entries). The script zigzags every span
# between two labelled points that is long enough to hold the pattern, and ramps the depth to zero at
# open ends so a trench surfaces instead of ending in a wall.
#
# Fire-trench rows follow the contour: each is 600 - BluffWaviness(x) * 15, and each bluff entry is
# 690 - BluffWaviness(x) * 15. Re-derive both if the Profile or BluffWaviness in the heightmap generator
# changes. Bunker rows come from PlaceHeroPieces.py BUNKERS (rear wall at y + depth/2); outpost rows from
# PlaceInfantryPositions.py POSITIONS.
# All numbers tentative per CLAUDE.md.

param(
    [string]$OutputPath = (Join-Path $PSScriptRoot "TrenchCenterline.json")
)

$ChannelWidth = 4.0
$ChannelDepth = 1.2
$ChannelShoulder = 1.0
$NodeSpacing = 10.0

$ZigzagLegAdvance = 8.0
$ZigzagSwing = 6.0
$ZigzagMinimumSpan = 24.0
$OpenEndRampLength = 8.0

$Route = @(
    @{ id = "fire_trench"; kind = "fire"; open_end = $false; points = @(
        @(150.0, 589.1, "line_end_left"),
        @(200.0, 591.2, "junction_comm_left"),
        @(260.0, 594.9, ""),
        @(320.0, 599.3, ""),
        @(355.0, 602.1, "junction_sap_left"),
        @(420.0, 606.7, ""),
        @(480.0, 610.0, ""),
        @(510.0, 611.1, "junction_comm_centre_and_sap_centre"),
        @(570.0, 612.2, ""),
        @(630.0, 611.5, ""),
        @(655.0, 610.8, "junction_sap_right"),
        @(720.0, 608.0, ""),
        @(800.0, 603.6, "junction_comm_right"),
        @(870.0, 600.0, "line_end_right")) },

    @{ id = "comm_left"; kind = "comm"; open_end = $true; points = @(
        @(200.0, 591.2, "junction_comm_left"),
        @(191.0, 610.0, "bypass_left_front"),
        @(191.0, 626.5, "bypass_left_rear"),
        @(200.0, 626.5, "door_approach_left"),
        @(200.0, 681.2, "bluff_entry_left")) },
    @{ id = "door_left"; kind = "door"; open_end = $true; points = @(
        @(200.0, 626.5, "door_approach_left"),
        @(200.0, 623.0, "bunker_door_left")) },

    @{ id = "comm_centre"; kind = "comm"; open_end = $true; points = @(
        @(510.0, 611.1, "junction_comm_centre_and_sap_centre"),
        @(500.0, 624.0, "bypass_centre_front"),
        @(500.0, 642.0, "bypass_centre_rear"),
        @(510.0, 642.0, "door_approach_centre"),
        @(510.0, 701.1, "bluff_entry_centre")) },
    @{ id = "door_centre"; kind = "door"; open_end = $true; points = @(
        @(510.0, 642.0, "door_approach_centre"),
        @(510.0, 638.5, "bunker_door_centre")) },

    @{ id = "comm_right"; kind = "comm"; open_end = $true; points = @(
        @(800.0, 603.6, "junction_comm_right"),
        @(809.0, 612.0, "bypass_right_front"),
        @(809.0, 626.5, "bypass_right_rear"),
        @(800.0, 626.5, "door_approach_right"),
        @(800.0, 693.6, "bluff_entry_right")) },
    @{ id = "door_right"; kind = "door"; open_end = $true; points = @(
        @(800.0, 626.5, "door_approach_right"),
        @(800.0, 623.0, "bunker_door_right")) },

    @{ id = "sap_left"; kind = "sap"; open_end = $true; points = @(
        @(355.0, 602.1, "junction_sap_left"),
        @(355.0, 550.0, "outpost_left")) },
    @{ id = "sap_centre"; kind = "sap"; open_end = $true; points = @(
        @(510.0, 611.1, "junction_comm_centre_and_sap_centre"),
        @(510.0, 560.0, "outpost_centre")) },
    @{ id = "sap_right"; kind = "sap"; open_end = $true; points = @(
        @(655.0, 610.8, "junction_sap_right"),
        @(655.0, 550.0, "outpost_right")) }
)

function New-Vertex([double]$X, [double]$Y, [string]$Label) {
    [pscustomobject]@{ x = $X; y = $Y; label = $Label; depth_fraction = 1.0 }
}

function Get-PolylineLength($Points) {
    $length = 0.0
    for ($i = 0; $i -lt $Points.Count - 1; $i++) {
        $length += [math]::Sqrt([math]::Pow($Points[$i + 1][0] - $Points[$i][0], 2) + [math]::Pow($Points[$i + 1][1] - $Points[$i][1], 2))
    }
    $length
}

function Get-PointAlongPolyline($Points, [double]$Distance) {
    for ($i = 0; $i -lt $Points.Count - 1; $i++) {
        $ax = $Points[$i][0]; $ay = $Points[$i][1]
        $bx = $Points[$i + 1][0]; $by = $Points[$i + 1][1]
        $edge = [math]::Sqrt(($bx - $ax) * ($bx - $ax) + ($by - $ay) * ($by - $ay))
        if ($Distance -le $edge -or $i -eq $Points.Count - 2) {
            $t = [math]::Min(1.0, $Distance / $edge)
            return @{ x = $ax + ($bx - $ax) * $t; y = $ay + ($by - $ay) * $t; dx = ($bx - $ax) / $edge; dy = ($by - $ay) / $edge }
        }
        $Distance -= $edge
    }
}

function Expand-SpanIntoZigzag($SpanPoints) {
    $spanLength = Get-PolylineLength $SpanPoints
    $legCount = [math]::Round($spanLength / $ZigzagLegAdvance)
    $inner = @()
    if ($spanLength -lt $ZigzagMinimumSpan -or $legCount -lt 2) {
        for ($i = 1; $i -lt $SpanPoints.Count - 1; $i++) {
            $inner += New-Vertex $SpanPoints[$i][0] $SpanPoints[$i][1] ""
        }
        return , $inner
    }
    $amplitude = $ZigzagSwing / 2.0
    for ($k = 1; $k -lt $legCount; $k++) {
        $along = Get-PointAlongPolyline $SpanPoints ($spanLength * $k / $legCount)
        $side = if ($k % 2 -eq 1) { 1.0 } else { -1.0 }
        $inner += New-Vertex ($along.x - $along.dy * $amplitude * $side) ($along.y + $along.dx * $amplitude * $side) ""
    }
    return , $inner
}

function Set-OpenEndRamp($Vertices) {
    $fromEnd = 0.0
    $rampLength = [math]::Min($OpenEndRampLength, (Get-PolylineLength ($Vertices | ForEach-Object { , @($_.x, $_.y) })))
    for ($i = $Vertices.Count - 1; $i -ge 0; $i--) {
        if ($i -lt $Vertices.Count - 1) {
            $fromEnd += [math]::Sqrt([math]::Pow($Vertices[$i + 1].x - $Vertices[$i].x, 2) + [math]::Pow($Vertices[$i + 1].y - $Vertices[$i].y, 2))
        }
        $t = [math]::Min(1.0, $fromEnd / $rampLength)
        $Vertices[$i].depth_fraction = $t * $t * (3 - 2 * $t)
    }
}

function Expand-Segment($Segment) {
    $points = $Segment.points
    $vertices = @(New-Vertex $points[0][0] $points[0][1] $points[0][2])
    $spanStart = 0
    for ($i = 1; $i -lt $points.Count; $i++) {
        if ($points[$i][2] -eq "" -and $i -lt $points.Count - 1) { continue }
        $vertices += Expand-SpanIntoZigzag $points[$spanStart..$i]
        $vertices += New-Vertex $points[$i][0] $points[$i][1] $points[$i][2]
        $spanStart = $i
    }
    if ($Segment.open_end) { Set-OpenEndRamp $vertices }
    $vertices
}

$invariant = [cultureinfo]::InvariantCulture
$segmentBlocks = foreach ($segment in $Route) {
    $vertexLines = foreach ($v in (Expand-Segment $segment)) {
        $label = if ($v.label -ne "") { ', "label": "{0}"' -f $v.label } else { "" }
        [string]::Format($invariant, '        {{ "x": {0:0.00}, "y": {1:0.00}, "depth_fraction": {2:0.000}{3} }}', $v.x, $v.y, $v.depth_fraction, $label)
    }
    @"
    {
      "id": "$($segment.id)",
      "kind": "$($segment.kind)",
      "points": [
$($vertexLines -join ",`n")
      ]
    }
"@
}

$json = @"
{
  "generated_by": "Tools/GenerateTrenchCenterline.ps1 - edit the Route table there, never this file",
  "units": "profile metres; 1 heightmap pixel = 1 m; world_uu = profile_m * 100 - 50400 on both axes",
  "depth_fraction": "share of channel.depth_m carved at this point; ramps to 0 where a trench surfaces",
  "channel": {
    "width_m": $($ChannelWidth.ToString($invariant)),
    "depth_m": $($ChannelDepth.ToString($invariant)),
    "shoulder_m": $($ChannelShoulder.ToString($invariant))
  },
  "node_spacing_m": $($NodeSpacing.ToString($invariant)),
  "segments": [
$($segmentBlocks -join ",`n")
  ]
}
"@

[System.IO.File]::WriteAllText($OutputPath, $json + "`n", (New-Object System.Text.UTF8Encoding $false))
Write-Host ("Trench centerline written to {0}" -f $OutputPath)
