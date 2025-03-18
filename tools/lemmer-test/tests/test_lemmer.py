import lemmer_test_common
import pytest
import yatest.common


OPTS = [
    ["-c", ],
    ["-t", ],
    ["--spellcheck", "-c", ],
    ["-c", "-p", ],
    ["-c", "--trans", ],
]


@pytest.mark.parametrize("opts", OPTS, ids=["".join(opts) for opts in OPTS])
def test_lemmer(opts):
    inp_path = lemmer_test_common.get_big_input()
    out_path = "".join(opts) + ".out"

    return lemmer_test_common.exec_lemmer_cmd(opts, inp_path, out_path)


def test_lemmer_automorphology():
    opts = ["-c", "-w", "-m", "est,eng", "-p", "--automorphology", "est=" + yatest.common.binary_path("search/wizard/data/wizard/Automorphology/est") + "/"]
    inp_path = lemmer_test_common.get_big_input()
    out_path = "with_est_automorphology.out"

    return lemmer_test_common.exec_lemmer_cmd(opts, inp_path, out_path)
