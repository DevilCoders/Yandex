#!/usr/bin/env python3

import pathlib
import subprocess


def check_target(target_path):
    print("Processing {}".format(target_path))
    subprocess.call(
            ["terraform", "plan"],
            cwd=target_path
            )
    

def check_all_targets():
    # globing by targets/<group>/<target>/<environment>
    for target_path in pathlib.Path('.').glob('targets/*/*/*'):
        if not target_path.is_dir():
            continue
        target_path = str(target_path)
        if 'common' not in target_path:
            check_target(target_path) 


def main():
    check_all_targets()


if __name__ == '__main__':
    main()
