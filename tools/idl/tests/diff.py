import difflib
import filecmp
import os
import sys
import tarfile
import yatest.common


IDLS_ROOT = 'tests/archive/idls/'


def print_diff(file1, file2):
    with open(file1, 'rb') as f1:
        with open(file2, 'rb') as f2:
            sys.stderr.writelines(difflib.unified_diff(f1.readlines(), f2.readlines(), file1, file2, n=5))
    sys.stderr.write('\n')


def print_current_directories(dir1, dir2):
    sys.stderr.write('>>>> comparing directories {}, {}:\n\n'.format(dir1, dir2))


def are_equal_dir_trees(dir1, dir2):
    dircmp = filecmp.dircmp(dir1, dir2)
    if dircmp.diff_files or dircmp.left_only or dircmp.right_only or dircmp.funny_files:
        return False
    _, mismatch, errors = filecmp.cmpfiles(dir1, dir2, dircmp.common_files)
    if mismatch or errors:
        return False
    for common_dir in dircmp.common_dirs:
        if not are_equal_dir_trees(os.path.join(dir1, common_dir), os.path.join(dir2, common_dir)):
            return False
    return True


def print_tree_diff(dir1, dir2):
    printed_current_directories = False
    should_print_diff_for = set()

    dircmp = filecmp.dircmp(dir1, dir2)
    if dircmp.diff_files or dircmp.left_only or dircmp.right_only or dircmp.funny_files:
        if not printed_current_directories:
            print_current_directories(dir1, dir2)
            printed_current_directories = True
        print >>sys.stderr, '>>>> diff_files: {}, left_only: {}, right_only: {}, funny_files: {}\n'.format(
            sorted(dircmp.diff_files), sorted(dircmp.left_only),
            sorted(dircmp.right_only), sorted(dircmp.funny_files))
        should_print_diff_for |= set(dircmp.diff_files)

    _, mismatch, errors = filecmp.cmpfiles(dir1, dir2, dircmp.common_files)
    if mismatch or errors:
        if not printed_current_directories:
            print_current_directories(dir1, dir2)
            printed_current_directories = True
        print >>sys.stderr, '>>>> mismatch: {}, errors: {}\n'.format(sorted(mismatch), sorted(errors))
        should_print_diff_for |= set(mismatch)

    for f in sorted(list(should_print_diff_for)):
        print_diff(os.path.join(dir1, f), os.path.join(dir2, f))

    if printed_current_directories:
        sys.stderr.write('\n\n')

    for common_dir in dircmp.common_dirs:
        print_tree_diff(os.path.join(dir1, common_dir), os.path.join(dir2, common_dir))


def generate_idls(idls, archive_dir, output_dir, is_public):
    args = [
        yatest.common.binary_path('tools/idl/bin/tools-idl-app'),
        '--in-proto-root', os.path.join(archive_dir, 'protos'),
        '-F', os.path.join(archive_dir, 'frameworks'),
        '-I', os.path.join(archive_dir, 'idls'),
        '--base-proto-package', 'yandex.maps.proto',
        '--out-base-root', os.path.join(output_dir, 'base'),
        '--out-android-root', os.path.join(output_dir, 'android'),
        '--out-ios-root', os.path.join(output_dir, 'ios'),
    ]

    if is_public:
        args += ['--public']

    args += idls
    yatest.common.execute(args)


def compare_dirs(gold_dir, test_dir):
    if not are_equal_dir_trees(gold_dir, test_dir):
        print_tree_diff(gold_dir, test_dir)
        assert False, (
            'archive.tbz2 from Sandbox resource is not equal to the archive generated in the test.'
            ' Have a look at ./test-results/pytest/testing_out_stuff/stderr for diff details.'
        )


def test_diff():
    # archive_path = yatest.common.source_path('tools/idl/tests/archive.tbz2')
    archive_path = yatest.common.work_path('archive.tbz2')
    out_dir = yatest.common.work_path('output')
    with tarfile.open(archive_path, 'r:bz2') as tar:
        tar.extractall(path=out_dir)
        idls = sorted([x[len(IDLS_ROOT):] for x in tar.getnames() if x.startswith(IDLS_ROOT) and x.endswith('.idl')], reverse=True)

    archive_dir = os.path.join(out_dir, 'tests', 'archive')

    test_variants = [
        (os.path.join(archive_dir, 'all'), os.path.join(out_dir, 'test_all_dir'), False),
        (os.path.join(archive_dir, 'public'), os.path.join(out_dir, 'test_public_dir'), True)
    ]

    for gold_dir, test_dir, is_public in test_variants:
        generate_idls(idls, archive_dir, test_dir, is_public)
        compare_dirs(gold_dir, test_dir)
