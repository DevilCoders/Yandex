import copy

import jsonpickle

def InitFields(obj):
    for attr, typ in zip(obj.__slots__, obj.FIELD_TYPES):
        setattr(obj, attr, typ())


def AttrsToString(obj):
    return '\t'.join([str(typ(getattr(obj, atr))) for typ, atr in zip(obj.FIELD_TYPES, obj.__slots__)])


def FromString(obj, strVal):
    fields = strVal.split('\t', len(obj.__slots__) - 1)
    for i, val in enumerate(fields):
        setattr(obj, obj.__slots__[i], obj.FIELD_TYPES[i](val))


class JsonSerializable(object):
    def ToString(self):
        return jsonpickle.dumps(self)

    @classmethod
    def FromString(cls, s):
        res = jsonpickle.loads(s)
        if res.__class__ != cls:
            raise Exception, "Wrong class of loaded object"
        return res


class Flags(JsonSerializable):
    def __init__(self):
        self.wasXmlSearch = False
        self.wasShow = False
        self.wasImageShow = False
        self.wasAttempt = False
        self.wasSuccess = False
        self.wasEnteredHiddenImage = False
        self.isRobot = False

    def CombineWith(self, other):
        for atr in self.__dict__.keys():
            newVal = getattr(self, atr) or getattr(other, atr)
            setattr(self, atr, newVal)

    def Copy(self):
        return copy.deepcopy(self)


class Identify(JsonSerializable):
    def __init__(self):
        self.reqid = ''
        self.uidStr = ''
        self.ip = ''
        self.ts = ''

    def Copy(self):
        return copy.deepcopy(self)


class NumRedirData(JsonSerializable):
    def __init__(self):
        self.numRedirs = 0
        self.numRemovals = 0

    def Copy(self):
        return copy.deepcopy(self)


class CaptchaData(JsonSerializable):
    def __init__(self):
        self.Ident = Identify()
        self.Flags = Flags()


class Factors(JsonSerializable):
    def __init__(self):
        self.version = '0'
        self.factors = []

    def Version(self):
        return str(self.version)

    def AssignFactors(self, listOfFactors):
        try:
            self.factors = [float(x) for x in listOfFactors]
        except:
            self.factors = []

    def IsEmpty(self):
        return len(self.factors) == 0

    def Copy(self):
        return copy.deepcopy(self)


class CaptchaDataAndFactors(JsonSerializable):
    def __init__(self):
        self.Ident = Identify()
        self.Flags = Flags()
        self.Factors = Factors()

    def AssignCaptchaData(self, captchaData):
        self.Ident = captchaData.Ident.Copy()
        self.Flags = captchaData.Flags.Copy()


class FinalCaptchaData(JsonSerializable):
    def __init__(self):
        self.Ident = Identify()
        self.Flags = Flags()
        self.NumRedir = NumRedirData()
        self.Factors = Factors()


    def Copy(self):
        return copy.deepcopy(self)


    def AssignFlags(self, flags):
        self.Flags = flags.Copy()


    def AssignCaptchaDataAndFactors(self, data):
        self.Ident = data.Ident.Copy()
        self.Flags = data.Flags.Copy()
        self.Factors = data.Factors.Copy()


    def AssignNumRedirs(self, redirData):
        self.NumRedir = redirData.Copy()


class RndReqDataRaw(JsonSerializable):
    def __init__(self):
        self.reqid = ''
        self.ip = ''
        self.uidStr = ''
        self.service = ''
        self.request = ''

    def Copy(self):
        return copy.deepcopy(self)


class TweakFlags(JsonSerializable):
    def __init__(self):
        self.numDocs = 0
        self.haveSyntax = 0
        self.haveRestr = 0
        self.quotes = 0
        self.cgiUrlRestr = 0


class RndReqData(JsonSerializable):
    def __init__(self):
        self.Raw = RndReqDataRaw()
        self.TweakFlags = TweakFlags()
        self.NumRedir = NumRedirData()
        self.Flags = Flags()

