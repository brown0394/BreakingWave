"""Aggregates the CSVs the in-game FPlaytestRecorder writes to Saved/Playtests/.

Run with plain system Python (no editor needed):
    python Tools/AnalyzePlaytests.py [path-to-Playtests-dir]

Prints per-life and aggregate stats, the session advance envelope, and the takeover
give-back; renders heatmap PNGs if matplotlib is installed.

The unit is a LIFE, not an attempt: since the death-transition pass a death hands the
player a nearby living ally instead of resetting to the craft, so a session is one
continuous push made of many lives. Sessions recorded before that pass used run_start /
run_abort and are still read here — for those, every life simply began at the craft.
"""

import glob
import os
import statistics
import sys
from collections import defaultdict

try:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    HAVE_MPL = True
except ImportError:
    HAVE_MPL = False

ZONE_BOUNDARIES_Y = [-25400.0, -19400.0, -11400.0, -2400.0, 7600.0, 15600.0]
# The shooter id in the gun column: MG bunkers are 0..n, infantry soldiers are offset above this
INFANTRY_SHOOTER_ID_BASE = 1000
ZONE_NAMES = ["Z0 landing", "Z1 waterline", "Z2 obstacles", "Z3 upper beach", "Z4 defense line"]
BEACH_X_RANGE = (-51000.0, 51000.0)
BEACH_Y_RANGE = (-27000.0, 17000.0)


def zone_at_y(y):
    zone = 0
    for i in range(1, len(ZONE_BOUNDARIES_Y)):
        if y >= ZONE_BOUNDARIES_Y[i]:
            zone = i
    return min(zone, len(ZONE_NAMES) - 1)


def read_text(path):
    with open(path, "rb") as f:
        raw = f.read()
    if raw.startswith(b"\xff\xfe"):
        return raw.decode("utf-16")
    return raw.decode("utf-8", errors="replace")


def parse_extra(extra):
    out = {}
    for pair in extra.strip().split(";"):
        if "=" in pair:
            key, value = pair.split("=", 1)
            out[key] = value
    return out


def parse_session(path):
    settings = {}
    runs = defaultdict(list)
    for line in read_text(path).splitlines():
        parts = line.split(",", 7)
        if len(parts) != 8 or parts[0] == "t":
            continue
        try:
            row = {
                "t": float(parts[0]),
                "run": int(parts[1]),
                "event": parts[2],
                "x": float(parts[3]),
                "y": float(parts[4]),
                "z": float(parts[5]),
                "gun": int(parts[6]),
                "extra": parse_extra(parts[7]),
            }
        except ValueError:
            continue
        if row["event"] == "setting":
            key, _, value = parts[7].partition("=")
            settings[key] = value
        elif row["run"] > 0:
            runs[row["run"]].append(row)
    return settings, runs


LIFE_START_EVENTS = ("life_start", "run_start")
LIFE_END_EVENTS = ("death", "life_abort", "run_abort")


def summarize_run(session, run_index, events):
    start = next((e for e in events if e["event"] in LIFE_START_EVENTS), None)
    end = next((e for e in events if e["event"] in LIFE_END_EVENTS), None)
    if start is None:
        return None

    run = {
        "session": session,
        "run": run_index,
        "status": end["event"] if end else "partial",
        "start_y": start["y"],
        "splits": {},
        "samples": [],
        "death_pos": None,
    }
    active = [e for e in events if end is None or e["t"] <= end["t"]]

    for e in active:
        if e["event"] == "zone_cross":
            run["splits"][int(e["extra"]["zone"])] = float(e["extra"]["split"])
        elif e["event"] == "sample":
            run["samples"].append(e)
    if end is not None and end["event"] == "death":
        run["death_pos"] = (end["x"], end["y"])

    if end is not None and "dur" in end["extra"]:
        x = end["extra"]
        run["duration"] = float(x["dur"])
        run["max_y"] = float(x["maxy"])
        run["shots_at"] = int(x["shots_at"])
        run["hits"] = int(x["hits"])
        run["cracks"] = int(x["cracks"])
        run["whizzes"] = int(x.get("whizzes", "0"))
        run["adv_targeted_m"] = float(x["adv_targeted"]) / 100.0
        run["adv_clear_m"] = float(x["adv_clear"]) / 100.0
        run["nodmg"] = x["nodmg"] == "1"
        run["pshots"] = int(x.get("pshots", "0"))
        run["inf_down"] = int(x.get("inf_down", "0"))
        run["wounds"] = int(x.get("wounds", "0"))
        run["headshot"] = x.get("head", "0") == "1"
    else:
        run["duration"] = active[-1]["t"] - start["t"] if active else 0.0
        run["max_y"] = max((e["y"] for e in run["samples"]), default=start["y"])
        run["shots_at"] = sum(1 for e in active if e["event"] in ("shot", "inf_shot") and e["extra"].get("tgt") == "-1")
        run["hits"] = sum(1 for e in active if e["event"] == "hit_player")
        run["cracks"] = sum(1 for e in active if e["event"] == "crack")
        run["whizzes"] = sum(1 for e in active if e["event"] == "whizz")
        run["pshots"] = sum(1 for e in active if e["event"] == "pshot")
        run["inf_down"] = sum(1 for e in active if e["event"] == "inf_down")
        adv = {True: 0.0, False: 0.0}
        for prev, cur in zip(run["samples"], run["samples"][1:]):
            dy = cur["y"] - prev["y"]
            if dy > 0:
                adv[cur["extra"].get("targeted") == "1"] += dy
        run["adv_targeted_m"] = adv[True] / 100.0
        run["adv_clear_m"] = adv[False] / 100.0
        run["nodmg"] = any(
            e["event"] == "nodamage" and e["extra"].get("enabled") == "1" for e in active
        ) or any(e["event"] == "hit_player" and e["extra"].get("nodmg") == "1" for e in active)
        run["wounds"] = sum(
            1 for e in active if e["event"] == "hit_player" and e["extra"].get("nodmg") != "1")
        run["headshot"] = any(
            e["event"] == "hit_player" and e["extra"].get("head") == "1" for e in active)

    run["advance_m"] = (run["max_y"] - run["start_y"]) / 100.0
    run["start_zone"] = zone_at_y(run["start_y"])
    run["end_zone"] = zone_at_y(run["max_y"])
    run["inf_shots_at"] = sum(
        1 for e in active if e["event"] == "inf_shot" and e["extra"].get("tgt") == "-1")
    run["inf_hits"] = sum(
        1 for e in active if e["event"] == "hit_player" and e["gun"] >= INFANTRY_SHOOTER_ID_BASE)
    run["takeover"] = next(
        ({"given_back_m": float(e["extra"].get("given_back", "0")) / 100.0,
          "made": int(e["extra"]["made"]) if "made" in e["extra"] else None,
          "disc_allies": int(e["extra"]["disc_allies"]) if "disc_allies" in e["extra"] else None,
          "dist_m": float(e["extra"]["dist"]) / 100.0 if "dist" in e["extra"] else None,
          "to": (e["x"], e["y"]),
          "from_y": float(e["extra"].get("death_y", "0"))}
         for e in events if e["event"] == "takeover"), None)
    return run


def print_runs(all_runs):
    header = (f"{'session':<18}{'life':>5}{'status':>9}{'dur s':>8}{'from':>6}{'zone':>6}{'ground m':>10}"
              f"{'shots@':>8}{'hits':>6}{'hit %':>7}{'wnd':>5}{'hs':>4}{'cracks':>8}{'whizz':>7}{'fired':>7}"
              f"{'infdn':>7}{'adv tgt':>9}{'adv clr':>9}{'back m':>8}{'made':>6}{'disc':>6}  splits")
    print(header)
    print("-" * len(header))
    for r in all_runs:
        hit_pct = 100.0 * r["hits"] / r["shots_at"] if r["shots_at"] else 0.0
        splits = "  ".join(f"Z{z} {s:.1f}" for z, s in sorted(r["splits"].items()))
        tag = " [nodmg]" if r["nodmg"] else ""
        takeover = r.get("takeover")
        back = f"{takeover['given_back_m']:>8.0f}" if takeover else f"{'-':>8}"
        made = f"{('y' if takeover['made'] else 'n'):>6}" if takeover and takeover["made"] is not None else f"{'-':>6}"
        disc = f"{takeover['disc_allies']:>6}" if takeover and takeover["disc_allies"] is not None else f"{'-':>6}"
        print(
            f"{r['session']:<18}{r['run']:>5}{r['status']:>9}{r['duration']:>8.1f}"
            f"{r['start_zone']:>6}{r['end_zone']:>6}{r['advance_m']:>10.0f}"
            f"{r['shots_at']:>8}{r['hits']:>6}{hit_pct:>7.1f}{r.get('wounds', 0):>5}"
            f"{('y' if r.get('headshot') else ''):>4}{r['cracks']:>8}{r['whizzes']:>7}"
            f"{r['pshots']:>7}{r['inf_down']:>7}"
            f"{r['adv_targeted_m']:>9.0f}{r['adv_clear_m']:>9.0f}{back}{made}{disc}  {splits}{tag}"
        )


def print_session_envelope(all_runs):
    """Does the loop ratchet forward? Ground won per life against ground given back on takeover."""
    by_session = defaultdict(list)
    for r in all_runs:
        by_session[r["session"]].append(r)

    print("\nSESSION ADVANCE ENVELOPE")
    header = (f"{'session':<18}{'lives':>7}{'takeovers':>11}{'first y':>9}{'best y':>9}"
              f"{'net m':>8}{'back m':>8}{'made %':>9}{'disc med':>10}")
    print(header)
    print("-" * len(header))
    for session, runs in sorted(by_session.items()):
        takeovers = [r["takeover"] for r in runs if r.get("takeover")]
        first_y = min(r["start_y"] for r in runs)
        best_y = max(r["max_y"] for r in runs)
        back = sum(t["given_back_m"] for t in takeovers)
        made = [t["made"] for t in takeovers if t["made"] is not None]
        discs = [t["disc_allies"] for t in takeovers if t["disc_allies"] is not None]
        made_pct = f"{100.0 * sum(made) / len(made):>9.0f}" if made else f"{'-':>9}"
        disc_med = f"{statistics.median(discs):>10.1f}" if discs else f"{'-':>10}"
        print(f"{session:<18}{len(runs):>7}{len(takeovers):>11}{first_y:>9.0f}{best_y:>9.0f}"
              f"{(best_y - first_y) / 100.0:>8.0f}{back:>8.0f}{made_pct}{disc_med}")

    all_takeovers = [r["takeover"] for r in all_runs if r.get("takeover")]
    if all_takeovers:
        made = [t["made"] for t in all_takeovers if t["made"] is not None]
        if made:
            print(f"\na real ally was inside the disc on {len(made) - sum(made)} of {len(made)} takeovers; "
                  f"the other {sum(made)} were spawned at the fog edge")
            dists = [t["dist_m"] for t in all_takeovers if t["dist_m"] is not None]
            if dists:
                print(f"distance to the man you became: median {statistics.median(dists):.0f}m  max {max(dists):.0f}m")
        legacy = [t for t in all_takeovers if t["made"] is None]
        if legacy:
            print(f"\n{len(legacy)} takeovers predate Decision 041 (rear-expansion ladder); "
                  f"their give-back is not comparable")
        print(f"give-back per takeover: median {statistics.median(t['given_back_m'] for t in all_takeovers):.0f}m  "
              f"max {max(t['given_back_m'] for t in all_takeovers):.0f}m")


def print_aggregates(all_runs):
    clean = [r for r in all_runs if not r["nodmg"]]
    deaths = [r for r in clean if r["status"] == "death"]
    print(f"\nAGGREGATES — {len(all_runs)} lives, {len(clean)} clean, {len(deaths)} ended in a death")
    headshots = sum(1 for r in deaths if r.get("headshot"))
    if deaths and headshots:
        print(f"deaths by headshot: {headshots} of {len(deaths)} ({100.0 * headshots / len(deaths):.0f}%)")
    if deaths:
        durations = [r["duration"] for r in deaths]
        print(f"survival: median {statistics.median(durations):.1f}s  min {min(durations):.1f}s  max {max(durations):.1f}s")
        zones = defaultdict(int)
        for r in deaths:
            zones[r["end_zone"]] += 1
        print("died in: " + "  ".join(f"{ZONE_NAMES[z]} x{n}" for z, n in sorted(zones.items())))
        shots = sum(r["shots_at"] for r in deaths)
        hits = sum(r["hits"] for r in deaths)
        inf_shots = sum(r["inf_shots_at"] for r in deaths)
        inf_hits = sum(r["inf_hits"] for r in deaths)
        mg_shots, mg_hits = shots - inf_shots, hits - inf_hits
        print(f"MG vs player: {mg_shots} shots aimed, {mg_hits} hits ({100.0 * mg_hits / mg_shots:.2f}%)"
              if mg_shots else "MG vs player: no aimed shots")
        if inf_shots or inf_hits:
            print(f"infantry vs player: {inf_shots} shots aimed, {inf_hits} hits "
                  f"({100.0 * inf_hits / inf_shots:.2f}%)" if inf_shots else
                  f"infantry vs player: no aimed shots, {inf_hits} hits")
        adv_t = sum(r["adv_targeted_m"] for r in deaths)
        adv_c = sum(r["adv_clear_m"] for r in deaths)
        total = adv_t + adv_c
        if total > 0:
            print(f"advance while targeted by a live gun: {adv_t:.0f}m ({100.0 * adv_t / total:.0f}%) | while clear: {adv_c:.0f}m ({100.0 * adv_c / total:.0f}%)")
        fired = sum(r["pshots"] for r in deaths)
        downed = sum(r["inf_down"] for r in deaths)
        if fired or downed:
            print(f"player rifle: {fired} rounds fired, {downed} infantry downed")
    for zone in range(1, len(ZONE_NAMES)):
        splits = [r["splits"][zone] for r in clean if zone in r["splits"]]
        if splits:
            print(f"reach {ZONE_NAMES[zone]}: median {statistics.median(splits):.1f}s over {len(splits)} lives")


def print_setting_diffs(session_settings):
    if len(session_settings) < 2:
        return
    values = defaultdict(dict)
    for session, settings in session_settings.items():
        for key, value in settings.items():
            values[key][session] = value
    differing = {k: v for k, v in values.items() if len(set(v.values())) > 1}
    if differing:
        print("\nSETTINGS THAT DIFFER BETWEEN SESSIONS (tuning changes):")
        for key, per_session in sorted(differing.items()):
            print(f"  {key}: " + "  ".join(f"{s}={v}" for s, v in sorted(per_session.items())))


def draw_zone_lines(ax):
    for boundary in ZONE_BOUNDARIES_Y:
        ax.axhline(boundary, color="gray", linewidth=0.6, alpha=0.6)
    for zone, name in enumerate(ZONE_NAMES):
        mid = (ZONE_BOUNDARIES_Y[zone] + ZONE_BOUNDARIES_Y[zone + 1]) / 2
        ax.text(BEACH_X_RANGE[0] + 1500, mid, name, fontsize=7, color="gray", va="center")
    ax.set_xlim(*BEACH_X_RANGE)
    ax.set_ylim(*BEACH_Y_RANGE)
    ax.set_xticks([])
    ax.set_yticks([])


def render_maps(all_runs, sessions_events, out_path):
    fig, axes = plt.subplots(1, 3, figsize=(18, 8))

    ax = axes[0]
    draw_zone_lines(ax)
    ax.set_title("player paths, hits (o), deaths (x), takeover handoff (arrow)")
    for r in all_runs:
        xs = [e["x"] for e in r["samples"]]
        ys = [e["y"] for e in r["samples"]]
        ax.plot(xs, ys, linewidth=0.7, alpha=0.6)
    hits = [(e["x"], e["y"]) for events in sessions_events for e in events if e["event"] == "hit_player"]
    if hits:
        ax.scatter(*zip(*hits), s=18, marker="o", facecolors="none", edgecolors="orange")
    deaths = [r["death_pos"] for r in all_runs if r["death_pos"]]
    if deaths:
        ax.scatter(*zip(*deaths), s=40, marker="x", color="red")
    for r in all_runs:
        takeover = r.get("takeover")
        if takeover and r["death_pos"]:
            ax.annotate("", xy=takeover["to"], xytext=r["death_pos"],
                        arrowprops=dict(arrowstyle="->", color="deepskyblue", alpha=0.7, linewidth=0.8))

    ax = axes[1]
    draw_zone_lines(ax)
    ax.set_title("enemy fire concentration (bullet impacts, player fire excluded)")
    impacts = [(e["x"], e["y"]) for events in sessions_events for e in events
               if e["event"] == "impact" and e["gun"] != -1]
    if impacts:
        ax.hist2d(*zip(*impacts), bins=[80, 50], range=[BEACH_X_RANGE, BEACH_Y_RANGE], cmap="inferno", cmin=1)
        draw_zone_lines(ax)
    guns = defaultdict(list)
    soldiers = defaultdict(list)
    for events in sessions_events:
        for e in events:
            if e["event"] == "shot":
                guns[e["gun"]].append((e["x"], e["y"]))
            elif e["event"] == "inf_shot":
                soldiers[e["gun"]].append((e["x"], e["y"]))
    for gun, positions in guns.items():
        gx = statistics.median(p[0] for p in positions)
        gy = statistics.median(p[1] for p in positions)
        ax.scatter([gx], [gy], s=60, marker="^", color="cyan")
        ax.text(gx, gy + 800, f"MG{gun}", fontsize=8, color="cyan", ha="center")
    for soldier, positions in soldiers.items():
        sx = statistics.median(p[0] for p in positions)
        sy = statistics.median(p[1] for p in positions)
        ax.scatter([sx], [sy], s=30, marker="v", color="orange")

    ax = axes[2]
    draw_zone_lines(ax)
    ax.set_title("ally deaths (heat) + player deaths (x)")
    ally_deaths = [(e["x"], e["y"]) for events in sessions_events for e in events if e["event"] == "ally_death"]
    if ally_deaths:
        ax.hist2d(*zip(*ally_deaths), bins=[80, 50], range=[BEACH_X_RANGE, BEACH_Y_RANGE], cmap="viridis", cmin=1)
        draw_zone_lines(ax)
    if deaths:
        ax.scatter(*zip(*deaths), s=40, marker="x", color="red")

    fig.tight_layout()
    fig.savefig(out_path, dpi=130)
    print(f"\nmaps written to {out_path}")


def main():
    default_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "Playtests")
    playtest_dir = sys.argv[1] if len(sys.argv) > 1 else default_dir
    paths = sorted(glob.glob(os.path.join(playtest_dir, "session_*.csv")))
    if not paths:
        print(f"no session_*.csv found in {playtest_dir}")
        return

    all_runs = []
    sessions_events = []
    session_settings = {}
    for path in paths:
        session = os.path.basename(path).replace("session_", "").replace(".csv", "")
        settings, runs = parse_session(path)
        session_settings[session] = settings
        for run_index in sorted(runs):
            summary = summarize_run(session, run_index, runs[run_index])
            if summary is not None:
                all_runs.append(summary)
            sessions_events.append(runs[run_index])

    if not all_runs:
        print("sessions found but no lives recorded")
        return

    print_runs(all_runs)
    print_session_envelope(all_runs)
    print_aggregates(all_runs)
    print_setting_diffs(session_settings)

    if HAVE_MPL:
        render_maps(all_runs, sessions_events, os.path.join(playtest_dir, "analysis.png"))
    else:
        print("\nmatplotlib not installed — text summary only (pip install matplotlib for the heatmaps)")


if __name__ == "__main__":
    main()
