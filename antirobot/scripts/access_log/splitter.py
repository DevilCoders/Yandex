from functools import partial
import re

_escaped_symbol_re = r'\\[\\"tnr]|\\x[0-9a-f]{2}'

_escape_map = {
    '\\' : '\\',
    '"' : '"',
    't' : '\t',
    'n' : '\n',
    'r' : '\r',
}

def _replace_escaped_symbols(m):
    s = m.group()
    return chr(int(s[2:], 16)) if s[1] == 'x' else _escape_map[s[1]]

_unescape_symbols = partial(re.compile(_escaped_symbol_re).sub, _replace_escaped_symbols)

_endDelim = {
    '[': ('] ', 1),
    '"': ('" ', 1),
    ' ': (' ', 0),
    }

def CommonSplit(logStr, firstDelim = ' '):
    def FindEndDelimPos(delim, pos):
        while True:
            endPos = logStr.find(delim, pos)
            if endPos < 0:
                return endPos

            if delim[0] != ' ' and logStr[endPos - 1] == '\\':
                pos = endPos + 1
            else:
                return endPos

    logStr = logStr.strip().decode('latin-1')
    res = []

    pos = 0
    shift = 0

    endPos = 0
    endKind = firstDelim
    length = len(logStr)
    while True:
        (delim, shift) = _endDelim.get(endKind, (None, None))
        if delim is None:
            return res

        endPos = FindEndDelimPos(delim, pos) #logStr.find(delim, pos)
        if endPos < 0:
            break

        if delim[0] == '"':
            res.append(_unescape_symbols(logStr[pos + shift:endPos].encode('utf-8')))
        else:
            res.append(logStr[pos + shift:endPos].encode('utf-8'))

        pos = endPos + 1 + shift
        endKind = logStr[pos]
        if endKind not in('"', '['):
            endKind = ' '

    if pos < length:
        if shift > 0:
            res.append(logStr[pos + shift:-shift].encode('utf-8'))
        else:
            res.append(logStr[pos:].encode('utf-8'))

    return res


def SplitAccessLog(req):
    return CommonSplit(req, ' ')

def SplitMarketLog(req):
    return CommonSplit(req, '[')
