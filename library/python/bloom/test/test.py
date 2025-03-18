import random
import string

import pytest

from library.python import bloom


def generate_random_string():
    return ''.join(
        random.choice(string.ascii_uppercase + string.digits) for _ in range(10)
    ).encode('utf8')


@pytest.fixture(scope="function")
def random_string():
    return generate_random_string()


@pytest.fixture(scope="function")
def other_random_string():
    return generate_random_string()


def test_bloom_methods(random_string):
    bf = bloom.BloomFilter(100, 0.1)
    assert not bf.has(random_string)
    bf.add(random_string)
    assert bf.has(random_string)


def test_bloom_sugar_methods(random_string):
    bf = bloom.BloomFilter(100, 0.1)
    assert random_string not in bf
    bf += random_string
    assert random_string in bf


def test_bloom_dumps_loads(random_string):
    bf = bloom.BloomFilter(100, 0.1)
    assert random_string not in bf
    bf.add(random_string)
    assert random_string in bf
    loaded_bf = bloom.loads(bloom.dumps(bf))
    assert random_string in loaded_bf


def test_bloom_merge(random_string, other_random_string):
    bf_1 = bloom.BloomFilter(100, 0.1)
    bf_2 = bloom.BloomFilter(100, 0.1)

    assert random_string not in bf_1
    assert other_random_string not in bf_1
    bf_1 += random_string
    assert random_string in bf_1
    assert other_random_string not in bf_1

    assert random_string not in bf_2
    assert other_random_string not in bf_2
    bf_2 += other_random_string
    assert random_string not in bf_2
    assert other_random_string in bf_2

    bf_1.merge(bf_2)
    assert random_string in bf_1
    assert other_random_string in bf_1
