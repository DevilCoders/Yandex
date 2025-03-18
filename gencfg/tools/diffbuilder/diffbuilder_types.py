import copy
import json

import core.card.types as card_types

INDENT = "    "
MAX_ARRAY_ITEMS_IN_DIFF = 20


def mkindent(msg):
    lst = msg.split('\n')
    return '\n'.join(map(lambda x: "%s%s" % (INDENT, x), lst))


class EBasicTypes(object):
    BASIC = 0
    OLDNEW = 1
    ARR = 2  # list of strings


class TBasicValueType(object):
    def __init__(self, tp, **kwargs):
        self.type = tp
        if self.type == EBasicTypes.BASIC:
            self.value = kwargs['value']
        elif self.type == EBasicTypes.OLDNEW:
            self.oldvalue = kwargs['oldvalue']
            self.newvalue = kwargs['newvalue']
        elif self.type == EBasicTypes.ARR:
            self.value = kwargs['value']
        else:
            raise Exception("Unknown basic type <%s>" % self.type)

    def __str__(self):
        if self.type == EBasicTypes.BASIC:
            return "%s" % self.value
        elif self.type == EBasicTypes.OLDNEW:
            return "%s -> %s" % (self.oldvalue, self.newvalue)
        elif self.type == EBasicTypes.ARR:
            if len(self.value) > MAX_ARRAY_ITEMS_IN_DIFF:
                omitted_num = len(self.value) - MAX_ARRAY_ITEMS_IN_DIFF
                data = self.value[:MAX_ARRAY_ITEMS_IN_DIFF] + ['... (%d elements omitted)' % (omitted_num,)]
            else:
                data = self.value
            return '[{}]'.format(' '.join(str(x) for x in data))
        else:
            raise Exception("Unknown basic type <%s> in TBasicValueType" % self.type)

    def to_json(self):
        if self.type == EBasicTypes.BASIC:
            return self.value
        elif self.type == EBasicTypes.OLDNEW:
            return {'oldvalue': self.oldvalue, 'newvalue': self.newvalue}
        elif self.type == EBasicTypes.ARR:
            return self.value
        else:
            raise Exception("Unknown basic type <%s> in TBasicValueType" % self.type)


class TMyJsonEncoder(json.JSONEncoder):
    def default(self, o):
        if o.__class__ == TBasicValueType:
            return o.to_json()
        else:
            return json.JSONEncoder.default(self, o)


def get_field_diff(result, fieldname, oldvalue, newvalue):
    if oldvalue is None and newvalue is None:
        return

    itype = oldvalue.__class__ if oldvalue is not None else newvalue.__class__
    if itype in [card_types.Date, card_types.ByteSize]:
        itype = str  # card_node.Data can not be initialized without __init__ args

    if oldvalue is None:
        oldvalue = itype()
    if newvalue is None:
        newvalue = itype()

    if isinstance(oldvalue, list):
        olds = set(map(lambda x: str(x), oldvalue))
        news = set(map(lambda x: str(x), newvalue))
        if olds != news:
            result[fieldname] = dict()
            if len(news - olds):
                result[fieldname]['added'] = TBasicValueType(EBasicTypes.ARR, value=sorted(list(news - olds)))
            if len(olds - news):
                result[fieldname]['removed'] = TBasicValueType(EBasicTypes.ARR, value=sorted(list(olds - news)))
    else:
        if oldvalue != newvalue:
            result[fieldname] = TBasicValueType(EBasicTypes.OLDNEW, oldvalue=oldvalue, newvalue=newvalue)


# def format_basic_value(value):
#    if isinstance(value, tuple):
#        if len(value) != 2:
#            raise Exception("Found strange tuple <%s> when formatting basic value" % (value))
#        return "%s -> %s" % (value[0], value[1])
#    else:
#        return "%s" % (value)

class NoneSomething(object):
    def __init__(self, args):
        self.args = copy.copy(args)

    def __getitem__(self, item):
        if item in self.args:
            return self.args[item]
        raise KeyError('{}'.format(item))

    def __getattr__(self, name):
        if name in self.args:
            return self.args[name]
        return None

    def __str__(self):
        return str(self.args)


class EDiffTypes(object):
    COMMIT_DIFF = "commit_diff"
    GROUP_DIFF = "group_diff"
    TIER_DIFF = "tier_diff"
    SEARCHERLOOKUP_DIFF = "searcherlookup_diff"
    HW_DIFF = "hw_diff"
    INTLOOKUP_DIFF = "intlookup_diff"


class TDiffEntry(object):
    def __init__(self, diff_type, status, watchers, tasks, telechats, generated_diff, props=None):
        self.diff_type = diff_type
        self.status = status
        self.watchers = watchers
        self.tasks = tasks
        self.telechats = telechats
        self.generated_diff = generated_diff
        self.props = props if props is not None else {}

    def report_json(self, as_string=True):
        d = copy.deepcopy(self.generated_diff)
        d["diff_type"] = self.diff_type

        if as_string:
            return json.dumps(d, cls=TMyJsonEncoder)
        else:
            return d

    def recurse_report_text(self, d):
        result = []
        if issubclass(d.__class__, dict):
            generator = d.iteritems()
        elif issubclass(d.__class__, list):
            generator = enumerate(d)
        else:
            raise Exception("Found unknown type <%s> in TDiffEntry.report_text" % d.__class__)

        for k, v in generator:
            if issubclass(v.__class__, (dict, list)):
                result.append("%s:\n%s" % (k, mkindent(self.recurse_report_text(v))))
            else:
                if '\n' in str(v):  # format multiline value differently
                    result.append("%s:\n%s" % (k, mkindent(str(v))))
                else:
                    result.append("%s: %s" % (k, v))

        return "\n".join(result)

    def report_text(self):
        return self.recurse_report_text(self.generated_diff)
