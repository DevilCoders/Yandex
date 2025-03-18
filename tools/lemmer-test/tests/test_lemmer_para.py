import lemmer_test_common


def test_p_opt():
    command = ["-c", "-p"]

    inp_path = lemmer_test_common.get_big_input()
    out_path = "p_opt.out"

    return lemmer_test_common.exec_lemmer_cmd(command, inp_path, out_path)

