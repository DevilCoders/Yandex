import crcmod
import sys
import binascii

crc32bzip2 = crcmod.predefined.mkPredefinedCrcFun('crc-32-bzip2')


def crc32(data: bytes) -> bytes:
    return uint_to_bytes(crc32bzip2(data), length=4)


def fast_crc32(data: bytes) -> bytes:
    return uint_to_bytes(binascii.crc32(data), length=4)


def uint_to_bytes(i: int, length: int) -> bytes:
    return i.to_bytes(length=length, byteorder=sys.byteorder, signed=False)
