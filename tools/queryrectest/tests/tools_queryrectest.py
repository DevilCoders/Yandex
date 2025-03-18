# coding=utf-8
import os

import yatest.common

_GEODB_BIN_PATH = yatest.common.work_path('geodb.bin')


def get_command(data_dir):
    command = [
        yatest.common.binary_path("tools/queryrectest/tools_queryrectest"),
        "-w", os.path.join(data_dir, "queryrec.weights"),
        "-p", os.path.join(data_dir, "queryrec.filters"),
        os.path.join(data_dir, "queryrec.dict"),
    ]
    return command


def get_cmd():
    return get_command(data_dir=yatest.common.data_path("recognize/queryrec"))


def get_wiz_cmd():
    return get_command(data_dir=yatest.common.data_path("wizard/language"))


def run_queryrectest(inp_path, region=None, wizard=False, mode="full", additional_args=tuple()):
    if not wizard:
        command = get_cmd()
    else:
        command = get_wiz_cmd()
    command.extend(["-o", mode])

    if region:
        command.extend(["-d", region])
    command.extend(additional_args)

    inp_path = yatest.common.source_path(''.join(('tools/queryrectest/tests/data/', inp_path,)))
    out_path = yatest.common.test_output_path('output.txt')

    with open(inp_path) as inp, open(out_path, "w") as out:
        yatest.common.execute(command, stdin=inp, stdout=out)

    return yatest.common.canonical_file(out_path)


def test_default():
    return run_queryrectest("Russia.txt")


def test_ua():
    return run_queryrectest("Ukraine.txt", "ua")


def test_by():
    return run_queryrectest("Russia.txt", "by")


def test_kz():
    return run_queryrectest("Russia.txt", "kz")


def test_tr():
    return run_queryrectest("Turkey.txt", "tr")


def test_de():
    return run_queryrectest("Deutschland.txt", "de")


def test_at():
    return run_queryrectest("Deutschland.txt", "at")


def test_cz():
    return run_queryrectest("Czech.txt", "cz")


def test_it():
    return run_queryrectest("Italy.txt", "it")


def test_uniq_langs():
    return run_queryrectest("UniqAlphabetLangs.txt")


# wiz data tests
def test_wiz_default():
    return run_queryrectest("Russia.txt", wizard=True)


def test_wiz_ua():
    return run_queryrectest("Ukraine.txt", "ua", wizard=True)


def test_wiz_by():
    return run_queryrectest("Russia.txt", "by", wizard=True)


def test_wiz_kz():
    return run_queryrectest("Russia.txt", "kz", wizard=True)


def test_wiz_tr():
    return run_queryrectest("Turkey.txt", "tr", wizard=True)


def test_wiz_de():
    return run_queryrectest("Deutschland.txt", "de", wizard=True)


def test_wiz_com_ger_ul():
    return run_queryrectest("Deutschland.txt", "com", wizard=True, additional_args=["-u", "ger"])


def test_wiz_com_ger():
    return run_queryrectest("Deutschland.txt", "com", wizard=True)


def test_wiz_at():
    return run_queryrectest("Deutschland.txt", "at", wizard=True)


def test_wiz_cz():
    return run_queryrectest("Czech.txt", "cz", wizard=True)


def test_wiz_it():
    return run_queryrectest("Italy.txt", "it", wizard=True)


# special modes
def test_mode_prefered():
    return run_queryrectest("Russia.txt", mode="prefered")


def test_mode_main():
    return run_queryrectest("Russia.txt", mode="main")


def test_wiz_default_with_geodb():
    return run_queryrectest("Russia.txt", wizard=True, additional_args=['--geodb', _GEODB_BIN_PATH])
