import collections
import json
import sys

from antirobot.tools.yasm_stats import lib as yasm_stats_lib


def main():
    if len(sys.argv) != 2:
        print("Usage: yasm_stats path/to/service_config.json", file=sys.stderr)
        return 1

    with open(sys.argv[1]) as file:
        service_config = json.load(file)

    counter_ids = yasm_stats_lib.get_counter_ids(service_config)
    print("Total number of counters:", len(counter_ids))

    counter_id_freqs = collections.Counter(
        counter_id.rsplit(";", 1)[-1]
        for counter_id in counter_ids
    )

    sorted_freqs = sorted(
        counter_id_freqs.items(),
        key=lambda id_freq: (-id_freq[1], id_freq[0]),
    )
    print("\n".join(f"{counter_id}: {freq}" for counter_id, freq in sorted_freqs))


if __name__ == "__main__":
    sys.exit(main() or 0)
