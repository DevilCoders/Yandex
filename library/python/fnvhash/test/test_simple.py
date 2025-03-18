import library.python.fnvhash as fnv


def test_empty():
    assert fnv.hash64('') == 14695981039346656037
    assert fnv.hash32('') == 2166136261


def test_simple():
    assert fnv.hash64('1234567') == 2449551094593701855
    assert fnv.hash32('1234567') == 2849763999
