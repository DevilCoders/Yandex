# coding=utf-8

import yatest.common


def get_big_input():
    return "words.txt"


def exec_lemmer_cmd(cmd_args, inp_path, out_path):
    with open(inp_path) as inp, open(out_path, "w") as out:
        command = [
            yatest.common.binary_path("tools/lemmer-test/lemmer-test"),
        ]
        command.extend(cmd_args)

        yatest.common.execute(command, stdout=out, stdin=inp)
    return yatest.common.canonical_file(out_path)

