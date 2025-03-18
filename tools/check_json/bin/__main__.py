import os
import sys
import glob
import argparse
import simplejson as json


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("masks", nargs='+', help='List of file masks')
    parser.add_argument("-r", "--recurse", action='store_true')
    parser.add_argument("-w", "--weak-match", action='store_true', help="Don't treat pattern mismatch as error")

    return parser.parse_args()


def main(params):
    ok = True
    cwd = os.getcwd()
    matched = 0
    for root, dirs, files in os.walk(cwd):
        root = '' if root == cwd else os.path.relpath(root, cwd)

        for mask in params.masks:
            mask = os.path.join(root, mask) if root else mask
            file_names = glob.glob(mask)
            matched += len(file_names)

            if not file_names and not params.weak_match:
                print >>sys.stderr, "[[rst]]Can't find file names that match '{}'[[rst]]".format(mask)
                ok = False
                continue

            for file_name in file_names:
                with open(file_name) as f:
                    try:
                        json.load(f)
                        print "Validating {}: OK".format(file_name)
                    except Exception as e:
                        print >>sys.stderr, "[[rst]]Failed loading [[imp]]{}[[rst]]: [[bad]]{}[[rst]]".format(file_name, e)
                        ok = False

        if not params.recurse:
            break

    if not matched and params.weak_match:
        print >>sys.stderr, "[[rst]]Can't find any file names that match any pattern '{}'[[rst]]".format(params.masks)
        ok = False

    return ok


if __name__ == "__main__":
    sys.exit(0 if main(parse_args()) else 1)
