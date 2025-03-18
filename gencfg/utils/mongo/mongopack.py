#!/skynet/python/bin/python

class PackBuffer(object):
    def __init__(self):
        self.buf = [0] * 10
        self.offset = 0

    def _pack_bits(self, v, sz):
        if (self.offset + sz) > len(self.buf) * 8:
            self.buf += [0] * 10

        left = sz
        while left > 0:
            last_byte = self.offset / 8
            last_byte_left = 8 - self.offset % 8

            shift = min(left, last_byte_left)

            cur_byte_v = v / (1 << (sz - left)) % (1 << shift) * (1 << (8 - last_byte_left))

            self.buf[last_byte] += cur_byte_v

            left -= shift
            self.offset += shift

    def pack_int(self, v, sz):
        assert (0 <= v < (1 << sz))
        self._pack_bits(v, sz)

    def pack_percents(self, v, sz):
        assert (0 <= v <= 1.)

        v = float(v)

        intv = int(v * (1 << sz))
        intv = min(max(0, intv), (1 << sz) - 1)
        self._pack_bits(intv, sz)

    def pack_float(self, v, maxv, mantissa, exponenta):
        assert (0 <= v <= maxv)

        v = float(v)
        maxv = float(maxv)

        maxexp = 1 << exponenta
        fixedv = v / maxv
        deg = 0
        curmax = 1.0
        while deg < maxexp - 1 and fixedv < curmax / 2.:
            deg += 1
            curmax /= 2
        intv = int(fixedv * (1 << mantissa) * (1 << deg))

        self._pack_bits(intv, mantissa)
        self._pack_bits(deg, exponenta)

    def finish(self):
        assert (self.offset < len(self.buf) * 8)
        self.buf = self.buf[:(self.offset / 8 + int(bool(self.offset % 8)))]


class UnpackBuffer(object):
    def __init__(self, serialized_data):
        self.v = 0
        for i in range(len(serialized_data)):
            self.v += ord(serialized_data[i]) << (i * 8)
        self.offset = 0

    def _unpack_bits(self, sz):
        result = (self.v >> self.offset) % (1 << sz)
        self.offset += sz
        return result

    def unpack_int(self, sz):
        return self._unpack_bits(sz)

    def unpack_percents(self, sz):
        return float(self._unpack_bits(sz)) / (1 << sz)

    def unpack_float(self, maxv, mantissa, exponenta):
        mant = self._unpack_bits(mantissa)
        exp = self._unpack_bits(exponenta)
        return mant / float(1 << mantissa) / (1 << exp) * maxv

    def finish(self):
        pass


class Serializer(object):
    VERSION = 1

    def __init__(self, buf):
        self.buf = buf
        self.offset = 0

    @staticmethod
    def serialize(instances_data, version=VERSION):
        buf = PackBuffer()
        buf.pack_int(Serializer.VERSION, 5)
        if version == 1:
            buf.pack_int(len(instances_data), 8)
            for instance_data in instances_data:
                buf.pack_int(instance_data['port'], 16)
                buf.pack_int(instance_data['major_tag'], 8)
                buf.pack_int(instance_data['minor_tag'], 8)
                buf.pack_percents(instance_data['instance_cpu_usage'], 10)
                buf.pack_float(instance_data['instance_mem_usage'], 1024., 6, 4)
            buf.finish()

            return str(bytearray(buf.buf))
        else:
            raise Exception("Unsupported version %s for serialization")

    @staticmethod
    def deserialize(serialized_data):
        buf = UnpackBuffer(serialized_data)
        instances_data = []

        version = buf.unpack_int(5)
        if version == 1:
            n = buf.unpack_int(8)
            for i in range(n):
                instance_data = {
                    'port': buf.unpack_int(16),
                    'major_tag': buf.unpack_int(8),
                    'minor_tag': buf.unpack_int(8),
                    'instance_cpu_usage': buf.unpack_percents(10),
                    'instance_mem_usage': buf.unpack_float(1024., 6, 4)
                }
                instances_data.append(instance_data)

            buf.finish()

            return instances_data
        else:
            raise Exception("Unsupported version %s for deserialization")
