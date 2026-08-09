# Synthesizes the rifle placeholder sounds (shared rifle system pass): the player's
# semi-auto report, the enemy bolt rifle's distinct deeper report, the bolt cycle clack
# (the defender's audible window), the empty-mag dry click, and reload foley.
# Writes WAVs to SourceAssets/, imports to /Game/Audio. Real sounds replace these in the
# sound pass. The enemy report is deliberately a different voice from the player's —
# in a no-UI game, audio identity IS the information channel.
#
# Headless OK (pass -script as an ABSOLUTE path):
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<abs>/Tools/GenerateRifleAudio.py"
#
# Idempotent: re-running overwrites the WAVs and reimports.

import math
import os
import random
import struct
import wave

import unreal

SAMPLE_RATE = 44100
PROJECT_DIR = unreal.SystemLibrary.get_project_directory()
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets")
DEST_PATH = "/Game/Audio"

random.seed(43)


def write_wav(path, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    scale = 0.9 * 32767.0 / peak
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(b"".join(
            struct.pack("<h", int(max(-32767, min(32767, s * scale)))) for s in samples))


def noise_burst(length_s, decay_s, delay_s=0.0, gain=1.0, lowpass=0.0):
    total = int(SAMPLE_RATE * (delay_s + length_s))
    out = [0.0] * total
    start = int(SAMPLE_RATE * delay_s)
    smoothed = 0.0
    for i in range(start, total):
        t = (i - start) / SAMPLE_RATE
        raw = random.uniform(-1.0, 1.0) * math.exp(-t / decay_s)
        if lowpass > 0.0:
            smoothed += lowpass * (raw - smoothed)
            raw = smoothed
        out[i] = gain * raw
    return out


def mix(*layers):
    total = max(len(layer) for layer in layers)
    out = [0.0] * total
    for layer in layers:
        for i, s in enumerate(layer):
            out[i] += s
    return out


def tone(freq, length_s, decay_s, delay_s=0.0, gain=1.0):
    total = int(SAMPLE_RATE * (delay_s + length_s))
    out = [0.0] * total
    start = int(SAMPLE_RATE * delay_s)
    for i in range(start, total):
        t = (i - start) / SAMPLE_RATE
        out[i] = gain * math.sin(2.0 * math.pi * freq * t) * math.exp(-t / decay_s)
    return out


def player_shot_samples():
    crack = noise_burst(0.22, 0.009, gain=1.0)
    body = tone(240.0, 0.22, 0.03, gain=0.55)
    thump = tone(120.0, 0.22, 0.05, gain=0.4)
    return mix(crack, body, thump)


def enemy_shot_samples():
    boom = noise_burst(0.4, 0.02, gain=0.9, lowpass=0.35)
    body = tone(150.0, 0.4, 0.06, gain=0.8)
    thump = tone(85.0, 0.4, 0.1, gain=0.7)
    snap = noise_burst(0.4, 0.005, gain=0.5)
    return mix(boom, body, thump, snap)


def metal_click(delay_s, gain=1.0, ring_freq=2600.0):
    click = noise_burst(0.05, 0.004, delay_s=delay_s, gain=gain)
    ring = tone(ring_freq, 0.06, 0.012, delay_s=delay_s, gain=0.5 * gain)
    return mix(click, ring)


def bolt_cycle_samples():
    open_clack = metal_click(0.0, gain=1.0, ring_freq=2400.0)
    slide_back = noise_burst(0.1, 0.05, delay_s=0.06, gain=0.25, lowpass=0.5)
    close_clack = metal_click(0.28, gain=0.9, ring_freq=2900.0)
    return mix(open_clack, slide_back, close_clack)


def dry_click_samples():
    return metal_click(0.0, gain=0.6, ring_freq=3200.0)


def reload_samples():
    mag_out = metal_click(0.0, gain=0.8, ring_freq=2100.0)
    rattle = noise_burst(0.5, 0.3, delay_s=0.35, gain=0.12, lowpass=0.4)
    mag_in = metal_click(1.3, gain=1.0, ring_freq=2000.0)
    seat_tap = metal_click(1.5, gain=0.7, ring_freq=2400.0)
    action = metal_click(2.0, gain=0.9, ring_freq=2800.0)
    return mix(mag_out, rattle, mag_in, seat_tap, action)


def import_wav(filename):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SOURCE_DIR, filename)
    task.destination_path = DEST_PATH
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset_path = DEST_PATH + "/" + os.path.splitext(filename)[0]
    asset = unreal.load_asset(asset_path)
    if asset is None:
        raise RuntimeError("import failed: " + asset_path)
    return asset_path


def save_checked(asset_path):
    if not unreal.EditorAssetLibrary.save_asset(asset_path):
        raise RuntimeError("save_asset FAILED for " + asset_path +
                           " — check for zombie UnrealEditor-Cmd holding the file")


SOUNDS = [
    ("RifleShotPlayer.wav", player_shot_samples),
    ("RifleShotEnemy.wav", enemy_shot_samples),
    ("RifleBoltCycle.wav", bolt_cycle_samples),
    ("RifleDryClick.wav", dry_click_samples),
    ("RifleReload.wav", reload_samples),
]


def main():
    os.makedirs(SOURCE_DIR, exist_ok=True)
    for filename, generator in SOUNDS:
        write_wav(os.path.join(SOURCE_DIR, filename), generator())
        unreal.log_warning("WAV written: %s" % filename)
    for filename, _ in SOUNDS:
        asset_path = import_wav(filename)
        save_checked(asset_path)
        unreal.log_warning("imported and saved: %s" % asset_path)


main()
