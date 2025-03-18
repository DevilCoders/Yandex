# coding: utf-8

import re
import operator
import gencfg
try:
    from typing import List
except ImportError:
    pass
from core.card.types import ByteSize


class SaaSByteSize(ByteSize):
    CUSTOM_ROUTER = re.compile('^in_(?P<in_unit>\w+)')

    def __init__(self, val, unsafe=False):
        if unsafe:
            self.value = int(val)
        elif isinstance(val, ByteSize):
            self.value = int(val.value)
        elif isinstance(val, (str, unicode)):
            m = re.match(self.SPLIT_REGEX, val)
            if m and m.group(2).upper() in ByteSize.SUFFIXES:
                val = float(m.group(1)) * ByteSize.SUFFIXES[m.group(2).upper()]
                if val % 1:
                    raise ValueError('Fractional bytesizes are not supported')
                else:
                    self.value = int(val)
            else:
                raise Exception('String {} can not be parsed as byte size'.format(val))
        else:
            raise ValueError('Could\'t construct SaaSByteSize from {}'.format(val))

    def reinit(self, val):
        self.__init__(val)

    @classmethod
    def from_int_bytes(cls, n):
        return cls(n, unsafe=True)

    def __repr__(self, suffix=None):
        divisor = 2 ** 10
        if suffix:
            suffix_multiplier = float(self.SUFFIXES.get(suffix.upper(), None))
            if suffix_multiplier:
                return '{:.2f} {}'.format(self.value/suffix_multiplier, suffix)
            else:
                raise ValueError('Unknown suffix {} for SaaSByteSize'.format(suffix))
        elif self.value == 0:
            return 'O B'
        else:
            bytesize_suffixes = sorted(self.SUFFIXES.items(), key=operator.itemgetter(1))
            bytesize_suffixes_len = len(bytesize_suffixes)
            bytesize_suffix_index = 0
            value = self.value
            while (bytesize_suffixes_len - bytesize_suffix_index) >= 2:
                new_val, reminder = divmod(value, divisor)
                if reminder == 0:
                    value = new_val
                    bytesize_suffix_index += 2
                else:
                    break
            return '{:d} {}'.format(value, bytesize_suffixes[bytesize_suffix_index][0].lower().capitalize())

    def __getattribute__(self, name):
        if name == 'text':
            return self.__repr__()
        router_regexp = object.__getattribute__(self, 'CUSTOM_ROUTER')
        suffixes = object.__getattribute__(self, 'SUFFIXES')
        m = router_regexp.match(name)
        if m:
            if m.group('in_unit').upper() == 'B':
                return object.__getattribute__(self, 'value')
            elif m.group('in_unit').upper() in suffixes.keys():
                return round(object.__getattribute__(self, 'value')/float(suffixes[m.group('in_unit').upper()]), 2)
            else:
                raise ValueError('Unknown measurement unit "{}" for SaaSByteSize'.format(m.group('in_unit')))
        else:
            return object.__getattribute__(self, name)

    def __add__(self, other):
        return self.__class__.from_int_bytes(self.value + other.value)

    def __sub__(self, other):
        return self.__class__.from_int_bytes(self.value - other.value)

    def __radd__(self, other):
        return self.__class__.from_int_bytes(other.value + self.value)

    def __rsub__(self, other):
        return self.__class__.from_int_bytes(other.value - self.value)

    def __iadd__(self, other):
        self.value += int(other.value)
        return self

    def __isub__(self, other):
        self.value -= int(other.value)
        return self

    def __mul__(self, other):
        if isinstance(other, (int, long)):
            return SaaSByteSize(self.value * other, unsafe=True)
        else:
            raise TypeError('unsupported operand type(s) for *: \'SaaSByteSize\' and \'{}\''.format(other.__class__.__name__))

    def __imul__(self, other):
        if isinstance(other, (int, long)):
            self.value *= other
            return self
        else:
            raise TypeError('unsupported operand type(s) for *=: \'SaaSByteSize\' and \'{}\''.format(other.__class__.__name__))
