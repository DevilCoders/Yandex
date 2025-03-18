import library.python.testing.system_info as si


def test_print_system_info():
    si_str = si.get_system_info()

    assert "CPU:" in si_str
    assert "MEM:" in si_str
    assert "swap:" in si_str
    assert "st:" in si_str
    assert "PackSent:" in si_str
