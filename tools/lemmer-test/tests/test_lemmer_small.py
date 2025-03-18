import yatest.common

import lemmer_test_common


def run_single_fix(use_fixlist, version_suffix, langs):
    command = [
        "-p",
        "-c",
    ]
    fixlist_path = yatest.common.source_path('tools/lemmer-test/tests/fixlist%s.txt' % (version_suffix, ))
    if use_fixlist:
        command.extend(["-f", fixlist_path])
        out_path = "with_fixlist%s.txt" % (version_suffix, )
    if langs:
        command.extend(["-m", ",".join(langs)])
        out_path = "no_fixlist%s.txt" % (version_suffix, )

    inp_path = yatest.common.source_path("tools/lemmer-test/tests/smallinp%s.txt" % (version_suffix, ))

    return lemmer_test_common.exec_lemmer_cmd(command, inp_path, out_path)


def test_with_fixlist():
    return run_single_fix(True, "", ['tr'])


def test_no_fixlist():
    return run_single_fix(False, "", ['tr'])


def test_with_fixlist2():
    return run_single_fix(True, "2", ['rus', 'ukr'])


def test_no_fixlist2():
    return run_single_fix(False, "2", ['rus', 'ukr'])


def test_tricky():
    command = [
        "-c",
    ]

    inp_path = yatest.common.source_path("tools/lemmer-test/tests/tricky.txt")
    out_path = "tricky.out"

    return lemmer_test_common.exec_lemmer_cmd(command, inp_path, out_path)


