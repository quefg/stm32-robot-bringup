#!/usr/bin/env python3
"""Extract IR_CAL captures, validate repeats and render a dependency-free SVG.

The bundled log contains only the measurement records from the project note.
No distance model or calibration-accuracy estimate is inferred from this data.
"""

import argparse
import csv
import hashlib
import json
import re
import statistics
from collections import defaultdict
from html import escape
from pathlib import Path


HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
DEFAULT_SOURCE = DATA / "ir-calibration-log.txt"
CSV_PATH = DATA / "ir-calibration.csv"
SUMMARY_PATH = DATA / "ir-summary.csv"
FIGURE_PATH = HERE.parent / "assets" / "ir-calibration.svg"
DISTANCES = (10, 15, 20, 25, 30, 40, 50, 60, 70, 80)
RECORD = re.compile(
    r"^\s*IR_CAL,([LR]),cm=(\d+),raw=(\d+),mV=(\d+)\s*$"
)
FIELDS = ("sample_order", "sensor", "distance_cm", "repeat_index", "raw_adc", "reported_mv")
COLORS = {"L": "#5DE1EF", "R": "#FFB86B"}


def extract(source):
    """Preserve capture order, duplicate values and each device-reported mV."""
    records = []
    clean_log = []
    repeats = defaultdict(int)
    for line in source.read_text(encoding="utf-8").splitlines():
        match = RECORD.fullmatch(line)
        if not match:
            if line.lstrip().startswith("IR_CAL,"):
                raise ValueError("An IR_CAL record is malformed; refusing a partial extraction")
            continue
        sensor, cm, raw, mv = match.groups()
        cm, raw, mv = int(cm), int(raw), int(mv)
        repeats[(sensor, cm)] += 1
        records.append({
            "sample_order": len(records) + 1,
            "sensor": sensor,
            "distance_cm": cm,
            "repeat_index": repeats[(sensor, cm)],
            "raw_adc": raw,
            "reported_mv": mv,
        })
        clean_log.append(line.strip())
    expected = {(sensor, cm) for sensor in ("L", "R") for cm in DISTANCES}
    if len(records) != 60 or set(repeats) != expected or set(repeats.values()) != {3}:
        raise ValueError("Expected 60 records: L/R x 10 recorded distances x 3 captures")
    if not all(0 <= r["raw_adc"] <= 4095 and r["reported_mv"] >= 0 for r in records):
        raise ValueError("ADC value outside 12-bit range, or negative reported mV")
    return records, clean_log


def summarize(records):
    grouped = defaultdict(list)
    for record in records:
        grouped[(record["sensor"], record["distance_cm"])].append(record)
    summaries = []
    for sensor in ("L", "R"):
        for cm in DISTANCES:
            captures = grouped[(sensor, cm)]
            raw = [r["raw_adc"] for r in captures]
            mv = [r["reported_mv"] for r in captures]
            summaries.append({
                "sensor": sensor,
                "distance_cm": cm,
                "captures": len(captures),
                "mean_raw_adc": statistics.mean(raw),
                "sample_sd_raw_adc": statistics.stdev(raw),
                "min_raw_adc": min(raw),
                "max_raw_adc": max(raw),
                "range_raw_adc": max(raw) - min(raw),
                "mean_reported_mv": statistics.mean(mv),
            })
    return summaries


def write_csv(path, fields, rows):
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def render_svg(records, summaries):
    """Plot actual x coordinates; overlapping captures are deliberately retained."""
    width, height = 1440, 1080
    colors = {"L": "#63A9CA", "R": "#74AF92"}
    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-labelledby="title description">',
        '<title id="title">IR sensor characterization: all 60 logged captures and repeated-capture spread</title>',
        '<desc id="description">Raw ADC versus known target distance for left and right Sharp GP2Y0A21YK sensors. Three captures at each of ten distances. Means connect observed values without a fitted model. Both traces are nonmonotonic at long distances; repeat ranges increase.</desc>',
        '<rect width="1440" height="1080" fill="#FFFFFF"/>',
        '<style>text{font-family:Inter,Arial,Helvetica,sans-serif} .muted{fill:#667784} .label{fill:#344054} .mono{font-family:ui-monospace,SFMono-Regular,Consolas,monospace}</style>',
    ]

    def text(x, y, value, size=16, fill="#344054", anchor="start", weight="400", extra=""):
        svg.append(f'<text x="{x:.2f}" y="{y:.2f}" font-size="{size}" fill="{fill}" text-anchor="{anchor}" font-weight="{weight}" {extra}>{escape(str(value))}</text>')

    def line(x1, y1, x2, y2, stroke="#E5ECEF", width=1, extra=""):
        svg.append(f'<line x1="{x1:.2f}" y1="{y1:.2f}" x2="{x2:.2f}" y2="{y2:.2f}" stroke="{stroke}" stroke-width="{width}" {extra}/>')

    text(72, 51, "STM32 ROBOT BRING-UP  /  BENCH DATA", 14, "#70818E", weight="600", extra='letter-spacing="2"')
    text(72, 106, "IR sensor characterization", 38, "#293843", weight="700")
    text(72, 141, "Sharp GP2Y0A21YK  ·  raw serial captures  ·  no fitted distance model", 17, "#667784")
    for x, big, small in ((72, "60", "LOGGED CAPTURES"), (366, "10", "DISTANCES / SENSOR"), (708, "3", "CAPTURES / DISTANCE")):
        text(x, 208, big, 32, "#293843", weight="700")
        text(x + 63, 204, small, 12, "#667784", weight="600", extra='letter-spacing="1"')
    text(1368, 191, "10–80 cm", 22, "#293843", anchor="end", weight="600")
    text(1368, 214, "RECORDED TARGET DISTANCES", 11, "#667784", anchor="end", extra='letter-spacing="1"')

    main = (120, 289, 1210, 393)
    left, top, plot_width, plot_height = main
    x = lambda cm: left + (cm - 5) / 80 * plot_width
    y = lambda raw: top + plot_height - raw / 4000 * plot_height
    svg.append(f'<rect x="72" y="250" width="1296" height="505" rx="8" fill="#FFFFFF" stroke="#DFE7EC"/>')
    text(98, 280, "01  RESPONSE", 14, "#4D606F", weight="700", extra='letter-spacing="1"')
    svg.append(f'<rect x="{x(57):.2f}" y="{top}" width="{x(83)-x(57):.2f}" height="{plot_height}" fill="#ADD8E8" fill-opacity="0.12"/>')
    for value in (0, 1000, 2000, 3000, 4000):
        line(left, y(value), left + plot_width, y(value))
        text(left - 15, y(value) + 5, f"{value:,}", 13, "#71818D", anchor="end")
    for cm in DISTANCES:
        line(x(cm), top, x(cm), top + plot_height, "#F0F4F6")
        text(x(cm), top + plot_height + 24, cm, 13, "#667784", anchor="middle")
    text(left + 12, top + 22, "Raw ADC counts", 12, "#71818D")
    text(left + plot_width, top + plot_height + 45, "Known target distance (cm)", 13, "#667784", anchor="end")

    for sensor in ("L", "R"):
        series = [s for s in summaries if s["sensor"] == sensor]
        points = " ".join(f'{x(s["distance_cm"]):.2f},{y(s["mean_raw_adc"]):.2f}' for s in series)
        svg.append(f'<polyline points="{points}" fill="none" stroke="{colors[sensor]}" stroke-width="1.8" stroke-opacity="0.9"/>')
        for s in series:
            xx = x(s["distance_cm"])
            line(xx, y(s["min_raw_adc"]), xx, y(s["max_raw_adc"]), colors[sensor], 1.3)
        for r in [r for r in records if r["sensor"] == sensor]:
            svg.append(f'<circle cx="{x(r["distance_cm"]):.2f}" cy="{y(r["raw_adc"]):.2f}" r="3.8" fill="{colors[sensor]}" fill-opacity="0.33" stroke="{colors[sensor]}" stroke-width="1.1"/>')
        for s in series:
            xx, yy = x(s["distance_cm"]), y(s["mean_raw_adc"])
            svg.append(f'<path d="M {xx:.2f},{yy-5:.2f} l 5,5 l -5,5 l -5,-5 Z" fill="{colors[sensor]}" stroke="#FFFFFF" stroke-width="1"/>')

    # Compact legend and a data-specific annotation occupy empty high-ADC space.
    for lx, sensor, label in ((810, "L", "LEFT · PC0 / ADC1_IN10"), (1091, "R", "RIGHT · PC1 / ADC1_IN11")):
        line(lx, 276, lx + 22, 276, colors[sensor], 3)
        text(lx + 30, 280, label, 12, colors[sensor], weight="600")
    bx, by = 861, 352
    svg.append(f'<rect x="{bx}" y="{by}" width="430" height="110" rx="6" fill="#FFFFFF" stroke="#E8C9CB"/>')
    text(bx + 20, by + 29, "LONG-RANGE NONMONOTONICITY", 13, "#B96B70", weight="700", extra='letter-spacing="0.6"')
    text(bx + 20, by + 54, "60–80 cm captures reverse or overlap.", 16, "#435563")
    text(bx + 20, by + 79, "Review spread before choosing a distance model.", 14, "#6A7A86")
    text(98, 747, "Circles: every logged capture (overlaps retained)    Diamonds + lines: arithmetic means; connecting lines are visual guides.", 12, "#71818D")

    # Repeat range is more interpretable here than an accuracy/confidence claim.
    svg.append('<rect x="72" y="775" width="1296" height="225" rx="8" fill="#FFFFFF" stroke="#DFE7EC"/>')
    text(98, 808, "02  REPEAT SPREAD", 14, "#4D606F", weight="700", extra='letter-spacing="1"')
    text(1330, 808, "Max − min of 3 captures · ADC counts", 13, "#667784", anchor="end")
    bar_top, bar_height, maximum = 833, 126, 350
    ybar = lambda value: bar_top + bar_height - value / maximum * bar_height
    for value in (0, 100, 200, 300):
        line(left, ybar(value), left + plot_width, ybar(value))
        text(left - 15, ybar(value) + 4, value, 12, "#71818D", anchor="end")
    for s in summaries:
        xx = x(s["distance_cm"]) + (-9 if s["sensor"] == "L" else 9)
        hh = max(1, s["range_raw_adc"] / maximum * bar_height)
        svg.append(f'<rect x="{xx-6:.2f}" y="{bar_top+bar_height-hh:.2f}" width="12" height="{hh:.2f}" rx="2" fill="{colors[s["sensor"]]}" fill-opacity="0.85"/>')
        if s["range_raw_adc"] >= 150:
            text(xx, ybar(s["range_raw_adc"]) - 7, s["range_raw_adc"], 11, colors[s["sensor"]], anchor="middle")
    for cm in DISTANCES:
        text(x(cm), 982, cm, 12, "#667784", anchor="middle")
    text(72, 1033, "mV values are device-reported; no independent voltage measurement or distance-accuracy result is established.", 14, "#667784")
    text(72, 1057, "Reproduce: python3 analysis/analyze_ir_calibration.py  ·  Source records and ordered CSV are included.", 12, "#8996A0")
    svg.append("</svg>\n")
    return "\n".join(svg)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE,
                        help="Text containing IR_CAL records; defaults to the bundled clean log")
    args = parser.parse_args()
    records, clean_log = extract(args.source)
    summaries = summarize(records)
    DATA.mkdir(parents=True, exist_ok=True)
    FIGURE_PATH.parent.mkdir(parents=True, exist_ok=True)
    DEFAULT_SOURCE.write_text("\n".join(clean_log) + "\n", encoding="utf-8")
    write_csv(CSV_PATH, FIELDS, records)
    write_csv(SUMMARY_PATH, tuple(summaries[0]), summaries)
    FIGURE_PATH.write_text(render_svg(records, summaries), encoding="utf-8")
    # Verify the serialized dataset, not just the in-memory parse.
    with CSV_PATH.open(encoding="utf-8", newline="") as stream:
        saved = list(csv.DictReader(stream))
    if saved != [{key: str(value) for key, value in row.items()} for row in records]:
        raise ValueError("CSV round-trip changed a capture value or its order")
    report = {
        "captures": len(records),
        "sensor_counts": {sensor: sum(r["sensor"] == sensor for r in records) for sensor in ("L", "R")},
        "distances_cm": list(DISTANCES),
        "captures_per_sensor_distance": 3,
        "csv_sha256": hashlib.sha256(CSV_PATH.read_bytes()).hexdigest(),
        "long_range": [s for s in summaries if s["distance_cm"] >= 60],
        "mean_increases": [
            {"sensor": sensor, "from_cm": a["distance_cm"], "to_cm": b["distance_cm"],
             "change_raw_adc": b["mean_raw_adc"] - a["mean_raw_adc"]}
            for sensor in ("L", "R")
            for series in [[s for s in summaries if s["sensor"] == sensor]]
            for a, b in zip(series, series[1:])
            if b["mean_raw_adc"] > a["mean_raw_adc"]
        ],
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
