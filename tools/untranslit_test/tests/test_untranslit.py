# coding: utf-8

from yatest import common


def test_untranslit():
    inp_file_path = common.data_path('lemmer/translit/translit_rus.txt')

    r1_out_file_path = common.output_path('res1.txt')
    with open(inp_file_path) as stdin, open(r1_out_file_path, 'w') as stdout:
        common.execute([common.binary_path('tools/untranslit_test/untranslit_test'), "-eyandex", "-d0", "-n"], stdin=stdin, stdout=stdout)

    r2_out_file_path = common.output_path('res2.txt')
    translit_rus_file_path = common.data_path('lemmer/translit/translit_rus_initial.txt')
    with open(r2_out_file_path, 'w') as stdout:
        common.execute(['perl', common.source_path('tools/untranslit_test/comp.pl'), translit_rus_file_path, inp_file_path, r1_out_file_path], stdout=stdout)
    return [common.canonical_file(r1_out_file_path), common.canonical_file(r2_out_file_path)]


def test_untranslit_index():
    inp_file_path = common.data_path('lemmer/translit/translit_index.txt')
    out_path = common.output_path("index.out.txt")

    with open(inp_file_path) as inp:
        with open(out_path, "w") as out:
            common.execute(
                [common.binary_path('tools/untranslit_test/untranslit_test'), "-e", "utf-8", "-i"],
                stdin=inp,
                stdout=out,
            )

    return common.canonical_file(out_path)

