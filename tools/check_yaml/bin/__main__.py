import argparse
import os
import sys

from tools.check_yaml.lib.checks import (
    check_simple_mask,
    check_subdir_mask,
    name_has_subdir,
)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("masks", nargs='+', help='List of file masks')
    parser.add_argument("-r", "--recurse", action='store_true')
    parser.add_argument("-w", "--weak-match", action='store_true', help="Don't treat mask mismatch as an error")

    return parser.parse_args()


def run_checks(masks, recurse, weak_match):
    have_subdirectories_in_masks = any(name_has_subdir(mask) for mask in masks)
    if recurse and have_subdirectories_in_masks:
        print >>sys.stderr, "[[rst]]--recurse and masks with subdirectories are incompatible[[rst]]"
        sys.exit(1)

    ok = True
    cwd = os.getcwd()
    matched = 0

    subdir_masks = [m for m in masks if name_has_subdir(m)]
    simple_masks = [m for m in masks if not name_has_subdir(m)]

    for mask in subdir_masks:
        matching_count, success = check_subdir_mask(mask, cwd, weak_match)
        matched += matching_count
        ok = ok and success

    if simple_masks:
        for absolute_subdir, _, files in os.walk(cwd):
            for mask in masks:
                matching_count, success = check_simple_mask(mask, cwd, absolute_subdir, files, weak_match)
                matched += matching_count
                ok = ok and success
            if not recurse:
                break

    if not matched and weak_match:
        print >>sys.stderr, "[[rst]]Can't find any file names that match any mask '{masks}'[[rst]]".format(masks=masks)
        ok = False

    return ok


def main():
    args = parse_args()
    ok = run_checks(args.masks, args.recurse, args.weak_match)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
