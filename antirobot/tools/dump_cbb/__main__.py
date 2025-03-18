import argparse
import json
import re

import requests


MAIN_RULE_CONFIG_KEYS = {
    "CbbFlagNotBanned": 499,
    "CbbFlagCutRequests": 423,
    "CbbFlagIgnoreSpravka": 328,
    "CbbFlagMaxRobotness": 329,
    "CbbFlagSuspiciousness": 511,
    "CbbFlagDegradation": 661,
    "CbbFlagBanSourceIp": 808,
    "CbbFlagBanFWSourceIp": 974,
}

RULE_CONFIG_KEYS = [
    "cbb_can_show_captcha_flag",
    "cbb_captcha_re_flag",
    "cbb_checkbox_blacklist_flag",
    "cbb_re_flag",
    "cbb_re_mark_flag",
    "cbb_re_mark_log_only_flag",
    "cbb_re_user_mark_flag",
]

LIST_ID_RE = re.compile("# *([0-9]+)")


def main():
    args = parse_args()

    with open(args.config) as main_config_file:
        main_config = main_config_file.readlines()

    with open(args.service_config) as config_file:
        config = json.load(config_file)

    group_ids = set()
    service_to_group_ids = {}

    found_main_keys = set()

    for line in main_config:
        tokens = [token.strip() for token in line.split("=", 1)]
        if len(tokens) == 2 and tokens[0] in MAIN_RULE_CONFIG_KEYS:
            found_main_keys.add(tokens[0])
            group_ids.add(int(tokens[1]))

    for key, value in MAIN_RULE_CONFIG_KEYS.items():
        if key not in found_main_keys:
            group_ids.add(value)

    for service_config in config:
        service_group_ids = []

        for key in RULE_CONFIG_KEYS:
            values = service_config[key]
            if isinstance(values, list):
                for value in values:
                    group_ids.add(value)
                    service_group_ids.append(value)
            else:
                group_ids.add(values)
                service_group_ids.append(value)

        service_to_group_ids[service_config["service"]] = service_group_ids

    api_handle = args.cbb_api + "/get_range.pl"

    id_to_text_list = {}
    list_ids = set()

    for group_id in group_ids:
        response = requests.get(
            api_handle,
            params={
                "flag": group_id,
                "with_format": "range_txt,rule_id"
            },
            headers={"Cookie": args.cookie},
        )

        if skip_error(group_id, response):
            continue

        text_list = []

        for line in response.text.splitlines():
            line = line.strip()
            if line == "":
                continue

            text_list.append(line)

            for list_id in LIST_ID_RE.findall(line):
                list_ids.add(list_id)

        id_to_text_list[group_id] = text_list

    id_to_re_list = {}

    for list_id in list_ids:
        response = requests.get(
            api_handle,
            params={
                "flag": list_id,
                "with_format": "range_re",
            },
            headers={"Cookie": args.cookie},
        )

        if skip_error(group_id, response):
            continue

        re_list = []

        for line in response.text.splitlines():
            line = line.strip()
            if line == "":
                continue

            re_list.append(line)

        id_to_re_list[list_id] = re_list

    with open(args.output, "w") as output_file:
        json.dump(
            {
                "txt": id_to_text_list,
                "re": id_to_re_list,
                "srv": service_to_group_ids,
            },
            output_file,
        )


def parse_args():
    parser = argparse.ArgumentParser(
        description="parse narwhal or market JWS token",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "--cookie",
        default="",
        help="cookie header",
    )

    parser.add_argument(
        "--cbb-api",
        default="https://cbb.n.yandex-team.ru/cgi-bin",
        help="CBB API URL prefix",
    )

    parser.add_argument(
        "--config",
        required=True,
        help="path to Antirobot config",
    )

    parser.add_argument(
        "--service-config",
        required=True,
        help="path to Antirobot service_config.json",
    )

    parser.add_argument(
        "--output",
        required=True,
        help="path to output",
    )

    return parser.parse_args()


def skip_error(group_id, response):
    if response.status_code == 400 and "invalid format" in response.text:
        return True

    try:
        response.raise_for_status()
    except Exception as exc:
        raise Exception(f"group {group_id}: {exc}")

    return False


if __name__ == "__main__":
    main()
