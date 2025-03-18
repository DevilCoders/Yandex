#!/usr/bin/env python3

import argparse
import codecs
import json
import logging


def update_crons(cron_ids, history_filters_config):
    history_filters_config_str = json.dumps(history_filters_config, sort_keys=True)
    for cron_id in cron_ids:
        file_path = "cron_{}.json".format(cron_id)
        with codecs.open(file_path, "r", "utf-8") as f:
            cron = json.load(f)
        for entry in cron["downloadParams"]:
            if entry["key"] == "history-filters-config":
                entry["value"] = history_filters_config_str
        cron["historyFilterConfigs"] = history_filters_config
        with codecs.open(file_path, "w", "utf-8") as f:
            json.dump(cron, f, indent=1, sort_keys=True)


def main():
    parser = argparse.ArgumentParser(description="Update some fields in crons.")
    parser.add_argument(
        "--history-filters-config",
        type=str,
        required=True,
        help="Path to JSON file with new history-filters-config",
    )
    parser.add_argument(
        "--cron-ids",
        type=str,
        nargs="+",
        help="cron ids",
    )
    args = parser.parse_args()

    logging.basicConfig(format='[%(levelname)s] %(asctime)s - %(message)s', level=logging.INFO)
    with codecs.open(args.history_filters_config, "r", "utf-8") as f:
        history_filters_config = json.load(f)
    update_crons(args.cron_ids, history_filters_config)


if __name__ == "__main__":
    main()
