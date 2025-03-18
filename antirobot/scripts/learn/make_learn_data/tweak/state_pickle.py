import datetime
import types
import urllib

class Serializable(object):
    __slots__ = []
    __Converters = {
        'str': (urllib.quote, urllib.unquote),
        'int': (str, int),
        'float': (str, float),
        'date': (lambda x: x.strftime('%Y%m%d'), lambda x: datetime.date(int(x[0:4]), int(x[4:6]), int(x[6:8]))),
        'datetime': (lambda x: x.strftime('%Y%m%d'), lambda x: datetime.date(int(x[0:4]), int(x[4:6]), int(x[6:8]))),
    }

    def Serialize(self):
        ToStr = lambda attr: self.__Converters[self.AttrTypes[attr]][0]
        return [ToStr(attr)(getattr(self, attr)) for attr in self.__slots__]

    @classmethod
    def Unserialize(cls, strList):
        FromStr = lambda attr: cls.__Converters[cls.AttrTypes[attr]][1]

        inst = cls()
        for attr, value in zip(cls.__slots__, strList):
            setattr(inst, attr, FromStr(attr)(value))
        return inst



def SerializeDict(dct):
    for k, v in dct.iteritems():
        yield '\t'.join([k] + v.Serialize())

def DumpDict(dct, fileHandle):
    for line in SerializeDict(dct):
        print >>fileHandle, line

def LoadDict(fileHandle, cls):
    res = {}
    for line in fileHandle:
        fs = line.rstrip().split('\t')
        key = fs[0]
        res[key] = cls.Unserialize(fs[1:])
    return res
