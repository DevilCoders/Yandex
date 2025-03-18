import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time


def main():
    args, prepare_argv = parse_args()

    prev_generation_time = None
    prev_bundle_info_path = args.prev_output / "bundle.json"
    if prev_bundle_info_path.exists():
        with prev_bundle_info_path.open() as prev_bundle_file:
            prev_generation_time = json.load(prev_bundle_file).get("generation_time")

    if args.output.exists():
        shutil.rmtree(args.output)
    args.output.mkdir()

    descriptor_key = args.descriptor_key.read_text().strip()

    if len(prepare_argv) == 0 or prepare_argv[0][0] == "-":
        prepare_argv = [find_prepare_executable()] + prepare_argv

    prepare_argv += ["--descriptor-key", args.descriptor_key]

    for i in range(args.size):
        cur_prepare_argv = prepare_argv + [
            "--generation-time", str(args.generation_time),
            "--output-script", str(args.output / f"greed.{args.generation_time}.{i}.js"),
        ]

        subprocess.check_call(cur_prepare_argv)

    bundle_info = {
        "generation_time": args.generation_time,
        "descriptor_key": descriptor_key,
    }

    if prev_generation_time is not None:
        bundle_info["prev_generation_time"] = prev_generation_time

    with (args.output / "bundle.json").open("w") as bundle_file:
        json.dump(bundle_info, bundle_file, indent=4)


def parse_args():
    parser = argparse.ArgumentParser(
        description="make a hypocrisy bundle",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--prev-output",
        type=Path,
        default=None,
        help="path to previous output (may be the same as --output)",
    )

    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="path to output bundle",
    )

    parser.add_argument(
        "--size",
        type=int,
        default=1,
        help="number of scripts to generate",
    )

    parser.add_argument(
        "--generation-time",
        type=int,
        default=int(time.time()),
        help="generation time (current time by default)",
    )

    parser.add_argument(
        "--descriptor-key",
        type=Path,
        required=True,
        help="path to descriptor encryption key file",
    )

    return parser.parse_known_args()


def find_prepare_executable():
    for prefix in (Path(sys.executable).absolute().parent, Path.cwd()):
        for suffix in (".", "../prepare"):
            path = prefix / suffix / "hypocrisy_prepare"
            if path.exists():
                return path.resolve()

    return None


if __name__ == "__main__":
    sys.exit(main() or 0)
