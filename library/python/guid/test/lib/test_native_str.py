import library.python.guid as lpg


def test_native_str():
    g = lpg.guid()

    assert isinstance(g, str)


def test_invariants():
    for i in range(1, 100):
        g = lpg.guid()

        assert g == lpg.to_string(lpg.parse(g))
