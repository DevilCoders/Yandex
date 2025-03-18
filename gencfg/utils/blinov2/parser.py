#!/skynet/python/bin/python

import sys
import re

from transport import EObjectTypes
import gencfg
from core.hosts import FakeHost, Host
from core.instances import FakeInstance, Instance


def _mkindent(s, indent="    "):
    return "\n".join(map(lambda x: "%s%s" % (indent, x), s.split('\n')))


class TBlinovParseError(Exception):
    def __init__(self, parser, message):
        message = """===================================================================
Got parse error:
%s
%s^ %s
===================================================================
""" % (parser.code, " " * parser.position, message)
        print message


class TBlinovCalcError(Exception): pass


class IBaseToken(object):
    def __init__(self, parser):
        self.parser = parser
        self.first = None
        self.second = None
        self.type = None

    def nud(self):
        raise NotImplementedError("Found not implemented function nud")

    def led(self, left):
        raise NotImplementedError("Found not implemented function led")

    def iterate(self, multitransport):
        raise NotImplementedError("Found not implemented function iterate")

    def counts(self, multitransport):
        pass

    def __repr__(self):
        return "(unimplemented)"


# noinspection PyAbstractClass
class ITreeElemToken(IBaseToken):
    def __init__(self, parser):
        super(ITreeElemToken, self).__init__(parser)

    def nud(self):
        raise TBlinovParseError(self.parser, "%s token can not be found here" % self.type)

    def counts(self, multitransport):
        return "%s <%s>\n%s\n%s" % (
        self.type, len(list(self.iterate(multitransport))), _mkindent(self.first.counts(multitransport)),
        _mkindent(self.second.counts(multitransport)))

    def __repr__(self):
        return "%s\n%s\n%s" % (self.type, _mkindent(str(self.first)), _mkindent(str(self.second)))


# noinspection PyAbstractClass
class IBinOpToken(ITreeElemToken):
    def __init__(self, parser):
        super(IBinOpToken, self).__init__(parser)

    def nud(self):
        raise TBlinovParseError(self.parser, "%s token can not be found here" % self.type)

    def led(self, left):
        self.first = left
        self.second = self.parser.parse_next(self.lbp)
        return self


class TIdentifierToken(IBaseToken):
    TYPES = {
        EObjectTypes.HOST: ['h@', 'host@'],
        EObjectTypes.HGROUP: ['H@', 'HOSTGROUP@', 'GROUP@', 'hostgroup@', 'group@'],
        EObjectTypes.STAG: ['S@', 'SHARDTAG@', 'STAG@', 'shardtag@', 'stag@'],
        EObjectTypes.SHARD: ['s@', 'shard@'],
        EObjectTypes.ITAG: ['I@', 'INSTANCETAG@', 'ITAG@', 'instancetag@', 'itag@', 'itag='],
        EObjectTypes.CONF: ['C@', 'CONF@', 'conf@'],
        EObjectTypes.DC: ['d@', 'dc@'],
        EObjectTypes.LINE: ['l@', 'line@'],
    }

    def __init__(self, parser, value):
        super(TIdentifierToken, self).__init__(parser)

        found = False
        for identifier_type, keynames in self.TYPES.iteritems():
            for keyname in keynames:
                if value.startswith(keyname):
                    self.itype = identifier_type
                    self.value = value[len(keyname):]

                    if self.value.find('@') > 0:
                        self.value, _, self.transport_type = self.value.partition('@')
                    else:
                        self.transport_type = 'default'

                    found = True
                    break

        if not found:
            if value.find('@') >= 0 or value.find('=') >= 0:
                raise Exception("Identifier <%s> identified as hostname, but hostname can not contain '@' or '='" % value)
            self.itype = EObjectTypes.HOST
            self.value = value

    lbp = 0

    def nud(self):
        return self

    def led(self, left):
        raise TBlinovParseError(self.parser, "Identiefier can not be found here")

    def iterate(self, multitransport):
        for instance in multitransport.iterate(self.itype, self.value, self.transport_type):
            yield instance

    def counts(self, multitransport):
        return "%s <%s>" % (repr(self), len(list(self.iterate(multitransport))))

    def __repr__(self):
        return "identifier (%s %s)" % (self.itype, self.value)


class TOrToken(IBinOpToken):
    def __init__(self, parser):
        super(TOrToken, self).__init__(parser)
        self.type = "OR"

    lbp = 10

    # FIXME: make normal iteration
    def iterate(self, multitransport):
        result = sorted(list(set(self.first.iterate(multitransport)) | set(self.second.iterate(multitransport))))
        for elem in result:
            yield elem


class TXorToken(IBinOpToken):
    def __init__(self, parser):
        super(TXorToken, self).__init__(parser)
        self.type = "XOR"

    lbp = 10

    # FIXME: make normal iteration
    def iterate(self, multitransport):
        result = sorted(list(set(self.first.iterate(multitransport)) ^ set(self.second.iterate(multitransport))))
        for elem in result:
            yield elem


class TMinusToken(IBinOpToken):
    def __init__(self, parser):
        super(TMinusToken, self).__init__(parser)
        self.type = "MINUS"

    lbp = 10

    # FIXME: make normal iteration
    def iterate(self, multitransport):
        result = sorted(list(set(self.first.iterate(multitransport)) - set(self.second.iterate(multitransport))))
        for elem in result:
            yield elem


# operation <MINUS> with instance iteator on left and host iterator on right
class THMinusToken(IBinOpToken):
    def __init__(self, parser):
        super(THMinusToken, self).__init__(parser)
        self.type = "HMINUS"

    lbp = 10

    # FIXME: make normal iteration
    def iterate(self, multitransport):
        left_instances = sorted(list(self.first.iterate(multitransport)))
        if len(left_instances) and not isinstance(left_instances[0], (FakeInstance, Instance)):
            raise TBlinovCalcError("Found non-instance %s in left of HMINUS token" % left_instances[0])

        right_hosts = set(self.second.iterate(multitransport))
        if len(right_hosts):
            if isinstance(next(iter(right_hosts)), (FakeInstance, Instance)):
                right_hosts = set(map(lambda x: x.host, right_hosts))
            elif not isinstance(next(iter(right_hosts)), (FakeHost, Host)):
                raise TBlinovCalcError(
                    "Found unsupported type <%s> in HMINUS operation processing" % (type(next(iter(right_hosts)))))

        for elem in filter(lambda x: x.host not in right_hosts, left_instances):
            yield elem


class TAndToken(IBinOpToken):
    def __init__(self, parser):
        super(TAndToken, self).__init__(parser)
        self.type = "AND"

    lbp = 5

    # FIXME: make normal iteration
    def iterate(self, multitransport):
        result = sorted(list(set(self.first.iterate(multitransport)) & set(self.second.iterate(multitransport))))
        for elem in result:
            yield elem


class TAndSkyToken(TAndToken):
    def __init__(self, parser):
        super(TAndSkyToken, self).__init__(parser)
        self.type = "AND_SKY"

    lbp = 20


class TLBraceToken(IBaseToken):
    def __init__(self, parser):
        super(TLBraceToken, self).__init__(parser)

    lbp = 0

    def nud(self):
        expr = self.parser.parse_next()
        if not isinstance(self.parser.token, TRBraceToken):
            raise TBlinovParseError(self.parser, "Here must be token ']'")
        self.parser.token, self.parser.position = self.parser.token_iter()
        return expr

    def led(self, left):
        raise TBlinovParseError(self.parser, "<[> token can not be found here")

    def __repr__(self):
        return "(lbrace)"


# needed just for tokenizer
class TRBraceToken(IBaseToken):
    def __init__(self, parser):
        super(TRBraceToken, self).__init__(parser)

    lbp = 0

    def nud(self):
        raise TBlinovParseError(self.parser, "<]> token can not be found here")

    def led(self, left):
        raise TBlinovParseError(self.parser, "<]> token can not be found here")

    def __repr__(self):
        return "(rbrace)"


# finishing token (last in every expression)
class TEndToken(IBaseToken):
    def __init__(self, parser):
        super(TEndToken, self).__init__(parser)

    lbp = 0


class Tokenizer(object):
    OPERATIONS = [
        ("OR", TOrToken),
        ("XOR", TXorToken),
        ("-", TMinusToken),
        (".", TAndSkyToken),
        ("AND", TAndToken),
        ("MINUS", TMinusToken),
        ("HMINUS", THMinusToken),
        ("[", TLBraceToken),
        ("]", TRBraceToken),
    ]

    SPACES_MATCHER = re.compile("\s*")
    IDENTIFIER_MATCHER = re.compile("([a-zA-Z_][a-zA-Z_0-9@=\-:/\.\~]*)")

    def __init__(self, parser):
        self.parser = parser
        self.prev_token = None
        self.current_token = None

    def skip_spaces(self, code, position):
        m = self.SPACES_MATCHER.match(code, position)
        return m.end(0)

    def need_missing_or_token(self, prev_token, current_token):
        if prev_token is None:  # start token
            return False
        if isinstance(prev_token, (TOrToken, TXorToken, TAndToken, TMinusToken, THMinusToken)):
            return False
        if isinstance(current_token, (TOrToken, TXorToken, TAndToken, TMinusToken, THMinusToken)):
            return False
        if isinstance(prev_token, TLBraceToken):
            return False
        if isinstance(current_token, TRBraceToken):
            return False
        return True

    def tokenize(self, code):
        position = self.skip_spaces(code, 0)
        while position != len(code):
            operation_found = False
            for token_name, token_class in self.OPERATIONS:
                if code[position:].startswith(token_name):
                    self.prev_token = self.current_token
                    self.current_token = token_class(self.parser)

                    if self.need_missing_or_token(self.prev_token, self.current_token):
                        yield TOrToken(self.parser), position - 1

                    yield self.current_token, position

                    operation_found = True
                    position += len(token_name)
                    break

            if not operation_found:
                m = self.IDENTIFIER_MATCHER.match(code, position)
                if not m:
                    raise TBlinovParseError(self.parser, "Can not find any valid token")

                self.prev_token = self.current_token
                self.current_token = TIdentifierToken(self.parser, m.group(0))

                if self.need_missing_or_token(self.prev_token, self.current_token):
                    yield TOrToken(self.parser), position - 1

                yield self.current_token, position

                position = m.end()

            position = self.skip_spaces(code, position)

        yield TEndToken(self.parser), len(code)


class BlinovParser(object):
    def __init__(self):
        self.token_iter = None
        self.token = None
        self.position = 0

    def parse(self, code):
        self.code = code
        self.token_iter = Tokenizer(self).tokenize(code).next
        self.token, self.position = self.token_iter()

        result = self.parse_next()

        if not isinstance(self.token, TEndToken):
            raise TBlinovParseError(self, "Got unexpected token")

        return result

    def parse_next(self, rbp=0):
        current_token = self.token
        self.token, self.position = self.token_iter()

        left = current_token.nud()
        while rbp < self.token.lbp:
            current_token = self.token
            self.token, self.position = self.token_iter()

            left = current_token.led(left)
        return left


if __name__ == '__main__':
    print Parser().parse(sys.argv[1])
