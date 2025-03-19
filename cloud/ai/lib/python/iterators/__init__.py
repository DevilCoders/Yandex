from itertools import islice


def chunk_iterator(sequence, size):
    if size <= 0:
        raise ValueError(f'expected size > 0, but {size} found')
    iterator = iter(sequence)
    return iter(lambda: list(islice(iterator, size)), [])
