import math
import os
import string
import uuid

from yc_common import validation


PREFIX_LEN = 15 // 5
SUFFIX_LEN = 85 // 5
ID_LEN = PREFIX_LEN + SUFFIX_LEN
ID_BITS = 15 + 85
SUFFIX_BYTES = math.ceil(85 / 8)
BASE_32_DIGIT_MASK = (1 << 5) - 1

_BASE_32_DIGITS = string.digits + string.ascii_lowercase


def generate_id(prefix, validate_prefix=False):
    if validate_prefix:
        validation.validate_id_prefix(prefix)

    random_bytes = os.urandom(SUFFIX_BYTES)
    random_int = int.from_bytes(random_bytes, byteorder="big") >> 3
    suffix = _to_base_32(random_int)
    return prefix + suffix[:SUFFIX_LEN].rjust(SUFFIX_LEN, "0")


def split_id(identifier):
    """Split into prefix and random part"""

    return identifier[:PREFIX_LEN], identifier[PREFIX_LEN:]


def convert_to_uuid(identifier):
    """Convert to uuid"""

    identifier_int = convert_to_int(identifier)
    identifier_uuid = str(uuid.UUID(int=identifier_int))

    return "23{}".format(identifier_uuid[2:])


def convert_to_int(identifier):
    """Convert identifier back to integer"""

    return int(identifier, 32)


def convert_from_uuid(uuid_str, id_len=ID_LEN):
    """Convert uuid backward to new identifier format"""

    if not uuid_str.startswith("23"):
        raise ValueError("{} doesn't start with '23'".format(uuid_str))

    uuid_value = uuid.UUID(uuid_str).int
    new_id_value = uuid_value & ((1 << ID_BITS) - 1)
    new_id = _to_base_32(new_id_value).rjust(id_len, "0")

    return new_id


def _to_base_32(number):
    result = ""
    while number != 0:
        digit = _BASE_32_DIGITS[number & BASE_32_DIGIT_MASK]
        number //= 32
        result = digit + result
    return result or "0"
