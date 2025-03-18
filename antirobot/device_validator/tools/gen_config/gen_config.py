#!/usr/bin/env python3
import argparse
import base64
import json
import os


def main():
    parser = argparse.ArgumentParser(description="generate config.")
    parser.add_argument('--config', type=str, nargs='+', help='folder with config')
    args = parser.parse_args()

    configs = {}

    for config in args.config:
        dir_name, appids = config.split(":")
        all_file = os.path.join(dir_name, 'all')
        priv_key_file = os.path.join(dir_name, 'ios-private-key')

        result = {}
        with open(priv_key_file) as f:
            result['key'] = base64.b64encode(f.read().encode('ascii')).decode('ascii')

        with open(all_file) as f:
            data = {}
            for line in f:
                key, value = line.split('=')
                data[key.strip()] = value.strip()

            result['kid'] = data.get('ios-key-id')
            result['iss'] = data.get('ios-team-id')

        for appid in appids.split(','):
            configs[appid] = result

    print(json.dumps(configs, indent=4, sort_keys=True))


if __name__ == "__main__":
    main()
