#!/usr/bin/env python3

import argparse
import json
import re

def parse_args():
    parser = argparse.ArgumentParser(description="Move's image id from hopper artifacts to target *.tf file (be careful, there is regex used inside)")
    parser.add_argument('--hopper-json', required=True, help='path to hopper artifact')
    parser.add_argument('--env', required=True, help='select image_id by environment')
    parser.add_argument('--folder-id', help='select image_id by folder_id')
    parser.add_argument('--target', required=True, help='path to target *.tf file')
    parser.add_argument('--target-var', required=True, help='name of tf variable')

    return parser.parse_args()

def load_json(path):
    with open(path) as json_file:
        return json.load(json_file)

def read_file(path):
    with open(path) as file:
        return file.read()

def write_file(path, content):
    with open(path, 'w') as file:
        return file.write(content)


def get_image_id(hopper, env, folder_id = None):
    for item in hopper["item"]:
        if item["environment"] == env and (folder_id is None or item["folder_id"] == folder_id):
            return item["artifact"]["id"]
    return None

if __name__ == '__main__':
    args = parse_args()
    hopper = load_json(args.hopper_json)
    image_id = get_image_id(hopper, args.env, args.folder_id)
    target_content = read_file(args.target)
    new_content = re.sub(f'({args.target_var}\\s*=\\s*)[^\\n]+', f'\\1"{image_id}"', target_content)
    write_file(args.target, new_content)