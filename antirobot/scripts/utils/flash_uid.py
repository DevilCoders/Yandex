import struct
import binascii


class Fuid:
    MAX_UINT32 = 4294967295

    def __init__(self, id):
        self.Id = int(id)
        self.Random = self.Id >> 32
        self.Timestamp = self.Id & self.MAX_UINT32

    @staticmethod
    def FromId(id):
        return Fuid(id)

    @staticmethod
    def FromCookieValue(cookVal):
        idStr = cookVal.split('.')[0]
        (lo, hi) = struct.unpack('>LL', binascii.unhexlify(idStr))
        return Fuid.FromId((hi << 32) + lo)
