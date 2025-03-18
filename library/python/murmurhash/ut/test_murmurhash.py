import murmurhash


def test_simple():
    assert murmurhash.hash64("abracadabra") == 17521050468336101133


# Test is taken from pyfasthash:
# https://github.com/flier/pyfasthash/blob/0f9f51de7cb90956d507090c37af5c2807861085/tests/test_murmur.py#L48-L52
def test_murmur2_x64_64a():
    hasher = murmurhash.hash64

    data = "test"
    bytes_hash = 3407684658384555107
    seed_hash = 14278059344916754999

    # Default seed
    assert bytes_hash == hasher(data)
    assert bytes_hash == hasher(data, seed=0)

    assert seed_hash == hasher(data, seed=bytes_hash)
    assert seed_hash == hasher(data, bytes_hash)
