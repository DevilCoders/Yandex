from __future__ import print_function

import argparse
import json
from sandbox import sdk2
from sandbox.projects.common.build.YaMake2 import YaMake2
from sandbox.projects.common.build.YaPackage import YaPackage

TASKS = (YaPackage, YaMake2)


def get_required_of_task(task):
    if issubclass(task, sdk2.Task):
        return [p.name for p in task.Parameters if p.required]
    else:
        return [p.name for p in task.input_parameters if p.required]


def get_defaults(task):
    if issubclass(task, sdk2.Task):
        return {p.name: p.default_value for p in task.Parameters}
    else:
        return {p.name: p.default_value for p in task.input_parameters}


def process_tasks(extractor):
    result = {task.type: extractor(task) for task in TASKS}
    print(json.dumps(result, indent=2, sort_keys=True))


def parse_args():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--required', action='store_true', required=False)
    group.add_argument('--defaults', action='store_true', required=False)
    return parser.parse_args()


def main():
    args = parse_args()
    if args.required:
        process_tasks(get_required_of_task)
    if args.defaults:
        process_tasks(get_defaults)


if __name__ == "__main__":
    main()
