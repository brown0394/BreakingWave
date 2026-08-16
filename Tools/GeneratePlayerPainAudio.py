# Synthesizes the player's pain grunt — the only wounded feedback in the death-transition
# pass (Decision 039: the persistent wounded presentation is deferred). Short, breathy,
# voiced: it has to read as a person, not as an impact, or a survived hit is indistinguishable
# from a miss. Writes the WAV to SourceAssets/, imports to /Game/Audio/PlayerPain.
#
# Headless OK (pass -script as an ABSOLUTE path):
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<abs>/Tools/GeneratePlayerPainAudio.py"
#
# Idempotent: re-running overwrites the WAV and reimports.

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

random.seed(71)


def write_wav(path, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    scale = 0.9 * 32767.0 / peak
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(b"".join(
            struct.pack("<h", int(max(-32767, min(32767, s * scale)))) for s in samples))


def envelope(t, length_s, attack_s):
    if t < attack_s:
        return t / attack_s
    return math.exp(-(t - attack_s) / (0.35 * length_s))


def voiced_grunt(length_s, start_freq, end_freq, gain):
    total = int(SAMPLE_RATE * length_s)
    out = [0.0] * total
    phase = 0.0
    for i in range(total):
        t = i / SAMPLE_RATE
        alpha = t / length_s
        freq = start_freq + (end_freq - start_freq) * alpha
        phase += 2.0 * math.pi * freq / SAMPLE_RATE
        glottal = math.sin(phase) + 0.45 * math.sin(2.0 * phase) + 0.2 * math.sin(3.0 * phase)
        out[i] = gain * glottal * envelope(t, length_s, 0.012)
    return out


def breath_noise(length_s, gain, lowpass):
    total = int(SAMPLE_RATE * length_s)
    out = [0.0] * total
    smoothed = 0.0
    for i in range(total):
        t = i / SAMPLE_RATE
        raw = random.uniform(-1.0, 1.0)
        smoothed += lowpass * (raw - smoothed)
        out[i] = gain * smoothed * envelope(t, length_s, 0.02)
    return out


def mix(*layers):
    total = max(len(layer) for layer in layers)
    out = [0.0] * total
    for layer in layers:
        for i, s in enumerate(layer):
            out[i] += s
    return out


def pain_samples():
    grunt = voiced_grunt(0.45, 150.0, 96.0, 1.0)
    breath = breath_noise(0.45, 0.28, 0.28)
    return mix(grunt, breath)


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


def main():
    os.makedirs(SOURCE_DIR, exist_ok=True)
    filename = "PlayerPain.wav"
    write_wav(os.path.join(SOURCE_DIR, filename), pain_samples())
    unreal.log_warning("WAV written: %s" % filename)
    asset_path = import_wav(filename)
    save_checked(asset_path)
    unreal.log_warning("imported and saved: %s" % asset_path)


main()
