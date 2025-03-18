import fnmatch
import glob
import os
import sys
import yaml


def name_has_subdir(mask):
    return os.path.split(mask)[0] != ""


def check_files(mask, cwd, absolute_dir, absolute_files, weak_match):
    result = True
    if not absolute_files and not weak_match:
        print >>sys.stderr, "[[rst]]Can't find file names that match '{mask}' in {relative_dir}[[rst]]".format(
            mask=mask, relative_dir=os.path.relpath(absolute_dir, cwd))
        result = False

    for absolute_name in absolute_files:
        relative_name = os.path.relpath(absolute_name, cwd)
        with open(relative_name) as f:
            try:
                yaml.safe_load(f)
                print "Validating {relative_name}: OK".format(relative_name=relative_name)
            except Exception as e:
                print >>sys.stderr, "[[rst]]Failed loading [[imp]]{relative_name}[[rst]]: [[bad]]{e}[[rst]]".format(
                    relative_name=relative_name, e=e)
                result = False

    return result


def check_subdir_mask(mask, cwd, weak_match):
    absolute_mask = os.path.join(cwd, mask)
    absolute_files = glob.glob(absolute_mask)
    return len(absolute_files), check_files(mask, cwd, cwd, absolute_files, weak_match)


def check_simple_mask(mask, cwd, absolute_dir, file_names, weak_match):
    matching_files = [name for name in file_names if fnmatch.fnmatch(name, mask)]
    absolute_files = [os.path.join(absolute_dir, name) for name in matching_files]
    return len(absolute_files), check_files(mask, cwd, absolute_dir, absolute_files, weak_match)
