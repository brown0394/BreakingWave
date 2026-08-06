# Synthesizes the Step 3 placeholder MG sounds (Decision 028/030 audio: fire loop +
# supersonic crack + ground impact), writes them to SourceAssets/, imports them to
# /Game/Audio, and marks the fire loop as looping. Real sounds replace these in the
# sound pass.
#
# Headless OK:
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/GenerateMGPlaceholderAudio.py"
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

ROUNDS_PER_SECOND = 20
LOOP_SECONDS = 1.0

random.seed(42)


def write_wav(path, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    scale = 0.9 * 32767.0 / peak
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(b"".join(
            struct.pack("<h", int(max(-32767, min(32767, s * scale)))) for s in samples))


def fire_loop_samples():
    total = int(SAMPLE_RATE * LOOP_SECONDS)
    period = SAMPLE_RATE // ROUNDS_PER_SECOND
    samples = [0.0] * total
    for shot in range(ROUNDS_PER_SECOND):
        start = shot * period
        amp = random.uniform(0.85, 1.0)
        for i in range(int(SAMPLE_RATE * 0.045)):
            idx = (start + i) % total
            t = i / SAMPLE_RATE
            noise = random.uniform(-1.0, 1.0) * math.exp(-t / 0.006)
            thump = 0.7 * math.sin(2.0 * math.pi * 130.0 * t) * math.exp(-t / 0.025)
            samples[idx] += amp * (noise + thump)
    return samples


def crack_samples():
    length = int(SAMPLE_RATE * 0.12)
    raw = []
    for i in range(length):
        t = i / SAMPLE_RATE
        if t < 0.0015:
            raw.append(random.uniform(-1.0, 1.0))
        else:
            raw.append(random.uniform(-1.0, 1.0) * math.exp(-(t - 0.0015) / 0.012))
    return [raw[i] - raw[i - 1] for i in range(1, length)]


def impact_samples():
    length = int(SAMPLE_RATE * 0.09)
    raw = []
    for i in range(length):
        t = i / SAMPLE_RATE
        noise = random.uniform(-1.0, 1.0) * math.exp(-t / 0.014)
        thud = 0.9 * math.sin(2.0 * math.pi * 95.0 * t) * math.exp(-t / 0.03)
        raw.append(noise + thud)
    smoothed = []
    for i in range(length):
        window = raw[max(0, i - 3):i + 1]
        smoothed.append(sum(window) / len(window))
    return smoothed


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
    return asset_path, asset


def save_checked(asset_path):
    if not unreal.EditorAssetLibrary.save_asset(asset_path):
        raise RuntimeError("save_asset FAILED for " + asset_path +
                           " — check for zombie UnrealEditor-Cmd holding the file")


def main():
    os.makedirs(SOURCE_DIR, exist_ok=True)

    loop_wav = os.path.join(SOURCE_DIR, "MGFireLoop.wav")
    crack_wav = os.path.join(SOURCE_DIR, "MGCrack.wav")
    impact_wav = os.path.join(SOURCE_DIR, "MGImpact.wav")
    write_wav(loop_wav, fire_loop_samples())
    write_wav(crack_wav, crack_samples())
    write_wav(impact_wav, impact_samples())
    unreal.log_warning("WAVs written: %s, %s, %s" % (loop_wav, crack_wav, impact_wav))

    loop_path, loop_asset = import_wav("MGFireLoop.wav")
    loop_asset.set_editor_property("looping", True)
    save_checked(loop_path)

    crack_path, _ = import_wav("MGCrack.wav")
    save_checked(crack_path)

    impact_path, _ = import_wav("MGImpact.wav")
    save_checked(impact_path)

    unreal.log_warning("Imported and saved: %s (looping), %s, %s"
                       % (loop_path, crack_path, impact_path))


main()
