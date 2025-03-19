def get_bits_indices(length, bit_length, bit_offset):
    assert bit_length > bit_offset
    if bit_length > length:
        return [(0, length)]
    indices = []
    for left in range(0, length - bit_length + 1, bit_offset):
        indices.append((left, left + bit_length))
    if (length - bit_length) % bit_offset != 0:
        left = length - bit_length - ((length - bit_length) % bit_offset) + bit_offset
        indices.append((left, length))
    return indices
