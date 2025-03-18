#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
import json
import difflib
import datetime
import itertools
import collections
import xml.sax.saxutils as xmlutils

if sys.hexversion >= 0x03000000:
    zip_longest = itertools.zip_longest
    string_types = str, bytes
else:
    zip_longest = itertools.izip_longest
    string_types = str, unicode


#: In object diffs: an object containing a part of the tree that did not exist.
#: In line diffs: the kind of line that should be highlighted green.
Add = collections.namedtuple('Add', ['value'])

#: In object diffs: an object that no longer exists.
#: In line diffs: the kind of line that is red.
Del = collections.namedtuple('Del', ['value'])

#: In object diffs: an unequal primitive value.
#: In line diffs: yellow lines.
Mod = collections.namedtuple('Mod', ['old', 'new'])

#: In line diffs: lines containing delimiters of modified objects.
Bracket = object()

#: In line diffs: lines that should be muted because they're just a side effect
#: of previous additions and removals of elements in an array.
IndexChange = object()

#: In line diffs: after an object field/array item, denotes the number of similar diffs
#: within the same object/array.
Count = object()

#: In line diffs: a set of characters that should be highlighted even more.
#: `kind` is either `Add` or `Del`.
Range = collections.namedtuple('Range', ['kind', 'start', 'end'])


def is_same_json_type(a, b):
    ta, tb = type(a), type(b)
    if ta is int:
        ta = float
    if tb is int:
        tb = float
    return ta is tb


def similarity(diff):
    '''Given an object diff, estimate how similar the objects were, from 0 to 1.'''
    if diff is None:
        return 1.0
    if isinstance(diff, (Add, Del)):
        return 0.0
    if isinstance(diff, Mod):
        # Looking at :obj:`diff` alone, it's a 100% difference. However, :class:`Mod`s only
        # appear as part  of a modified dict when the key did not change, but the value did.
        # Thus the key half is still the same.
        return 0.66 if is_same_json_type(diff.old, diff.new) else 0.33
    if isinstance(diff, dict):
        return sum(similarity(v) for v in diff.values()) / len(diff)
    if isinstance(diff, list):
        # The numerator is at most `2 * sum(not isinstance(v, (Add, Del)))` and is obviously
        # less than or equal (if there are no :class:`Add`s/:class:`Del`s) to the denominator,
        # which is just the sum of sizes of the "before" and "after" arrays. This means
        # :func:`diff` can treat the multiplier as constant, and the function as additive,
        # allowing the use of dynamic programming instead of brute force to maximize similarity.
        return 2 * sum(similarity(v) for v in diff) / sum(1 + (not isinstance(v, (Add, Del))) for v in diff)
    raise TypeError('invalid diff type {}'.format(type(diff)))


def _affects(diff):
    if isinstance(diff, dict):
        return frozenset((k,) + p for k, v in diff.iteritems() if v is not None for p in _affects(v))
    if isinstance(diff, list):
        return frozenset(p for v in diff if v is not None for p in _affects(v))
    return frozenset([(type(diff),)])


def group(tables):
    """
        Given an input to `render` (a list of tuples), group diffs by paths affected.
        In each list, the leading diff is a superset of every other one.

        :rtype: a list of lists of tuples.

    """
    ps = [[(_affects(t[-1]),) + t] for t in tables if t[-1] is not None]
    ps.sort(key=lambda x: len(x[0][0]))
    for i, p in enumerate(ps, 1):
        for q in ps[i:]:
            if p[0][0].issubset(q[0][0]):
                q.extend(p)
                p[:] = []
                break
    return [[q[1:] for q in p] for p in ps if p]


def timed_out(timeout):
    return timeout and datetime.datetime.now() >= timeout


def diff(a, b, timeout=None):
    '''Construct an object diff between json-encodable values.

        :rtype:
          * :class:`dict` -- the object is the same, but its fields differ. Values are object diffs.
          * :class:`list` -- this is the same array with some elements modified. Items are object diffs.
          * :class:`Add`/:class:`Del`/:class:`Mod` -- see their respective comments.
          * :class:`types.NoneType` -- the objects in that part of the tree have not changed.

    '''
    if a == b:
        return None
    if isinstance(a, dict) and isinstance(b, dict):
        a_keys = set(a.keys())
        b_keys = set(b.keys())
        result = {}
        for added in sorted(b_keys - a_keys):
            result[added] = Add(b[added])
        for deleted in sorted(a_keys - b_keys):
            result[deleted] = Del(a[deleted])
        for k in a_keys & b_keys:
            if timed_out(timeout):
                result[k] = Mod(a[k], 'diff timed out')
            else:
                result[k] = diff(a[k], b[k], timeout)
        return result
    if isinstance(a, list) and isinstance(b, list):
        diffs = [
            [
                # `(similarity of resulting array diff, a diff to append before the next step)`.
                # Addition is a step to the left, deletion is up, modification/no change is both.
                # (Similarity is multiplied by `(len(a) + len(b)) / 2`, but that does not concern `max`.)
                (0, Add(b[-j])) if j else (0, Del(a[-i])) if i else (0, None) for j in range(len(b) + 1)
            ] for i in range(len(a) + 1)
        ]
        for j, q in enumerate(reversed(b), 1):
            for i, p in enumerate(reversed(a), 1):
                if max(diffs[i][j - 1][0], diffs[i - 1][j][0]) >= diffs[i - 1][j - 1][0] + 1:
                    d = None  # forall x . 0 <= similarity(x) <= 1
                elif timed_out(timeout):
                    d = Mod(p, 'diff timed out')
                else:
                    d = diff(p, q, timeout)
                diffs[i][j] = max([
                    (diffs[i][j - 1][0], Add(q)),  # + similarity(Add(q)) [== 0]
                    (diffs[i - 1][j][0], Del(p)),  # + similarity(Del(p)) [also 0]
                    (diffs[i - 1][j - 1][0] + similarity(d), d),
                ], key=lambda x: x[0])
        i, j, result = len(a), len(b), []
        while i or j:
            _, d = diffs[i][j]
            result.append(d)
            i -= not isinstance(d, Add)
            j -= not isinstance(d, Del)
        return result
    return Mod(a, b)


def _json_lines(x, max_line_length=80, indent=4):
    '''Encode a json value on one line if it fits, else pretty-print it. Return a list of lines.'''
    u = json.dumps(x, sort_keys=True, ensure_ascii=False)
    # There's no way to put line breaks in primitives.
    if len(u) <= max_line_length or not isinstance(x, (dict, list)):
        return [u]
    return json.dumps(x, sort_keys=True, indent=indent, ensure_ascii=False).split(u'\n')


def _index(diff):
    """List of diffs -> ((old index, new index), diff); dict of diffs -> (key, value)."""
    if isinstance(diff, dict):
        for k, v in diff.items():
            if v is not None:
                yield k, v
    elif isinstance(diff, list):
        i = j = 0
        for item in diff:
            if item is not None:
                yield (i, j), item
            i += not isinstance(item, Add)
            j += not isinstance(item, Del)


def line_diff(diff, max_line_length=80, indent=4, min_equal_part=0.75, group_items=False):
    '''Construct the usual line-based diff from that weird "object diff".

        :param min_equal_part: only try to mark additions and deletions within strings
                               if their longest common subsequence is at least this long.

        :param group_items: within objects and arrays, group diffs by affected fields and only return
                            the first occurence in each group.

        :rtype:
            Iterator of `(kind, indent, text, ranges)` tuples.
              * `kind` is one of the values above.
              * `indent` is a multiple of the specified tab size.
              * `text` is a combination of "before" and "after" states.
              * Exactly which parts of `text` were removed or added is specified in `ranges`.

    '''
    if diff is None:
        pass
    elif isinstance(diff, Add):
        for line in _json_lines(diff.value, max_line_length, indent):
            yield Add, 0, line, []
    elif isinstance(diff, Del):
        for line in _json_lines(diff.value, max_line_length, indent):
            yield Del, 0, line, []
    elif isinstance(diff, Mod):
        if isinstance(diff.old, string_types) and isinstance(diff.new, string_types):
            line = ''
            xs = x, = [json.dumps(diff.old, ensure_ascii=False)]
            ys = y, = [json.dumps(diff.new, ensure_ascii=False)]
            ranges, equal, has_del, has_add = [], 0, False, False
            if max(len(x), len(y)) < max_line_length * 2:
                for op, start_a, end_a, start_b, end_b in difflib.SequenceMatcher(a=x, b=y).get_opcodes():
                    if op in ('replace', 'delete'):
                        ranges.append(Range(Del, len(line), len(line) + end_a - start_a))
                        line += x[start_a:end_a]
                        has_del = True
                    if op in ('replace', 'insert'):
                        ranges.append(Range(Add, len(line), len(line) + end_b - start_b))
                        line += y[start_b:end_b]
                        has_add = True
                    if op == 'equal':
                        equal += end_b - start_b
                        line += y[start_b:end_b]
                if not has_del or not has_add or equal * 2 > (len(x) + len(y)) * min_equal_part:
                    yield Mod, 0, line, ranges
                    return
        else:
            xs = _json_lines(diff.old, max_line_length, indent)
            ys = _json_lines(diff.new, max_line_length, indent)
            x, y = xs[0], ys[0]
        if len(xs) != 1 or len(ys) != 1:
            yield Mod, 0, '<type has changed>', []
            for line in xs:
                yield Del, indent, line, []
            for line in ys:
                yield Add, indent, line, []
            return
        # mark the space as None so that it is not displayed at all if the Mod is split into Del+Add during rendering
        yield Mod, 0, x + ' ' + y, [Range(Del, 0, len(x)), Range(None, len(x), len(x) + 1), Range(Add, len(x) + 1, len(x) + len(y) + 1)]
    elif isinstance(diff, (dict, list)):
        open, close = u"{}" if isinstance(diff, dict) else u"[]"
        yield Bracket, 0, open, []
        if group_items:
            items = sorted(g[0] + (len(g),) for g in group(_index(diff)))
        else:
            items = sorted((k, v, 1) for k, v in _index(diff))
        for k, v, count in items:
            # Collapse a nested object where each level has only one changed item.
            # Example: "a", {"b": [None, {"c": d}]} -> '"a"."b".1."c"', d
            path, end = [], None
            while v is not None:
                if isinstance(k, tuple):
                    old, new = k
                    path.append(
                        u"{}".format(new) if isinstance(v, Add) or old == new else
                        u"{}".format(old) if isinstance(v, Del) else
                        u"({} -> {})".format(old, new)
                    )
                else:
                    path.append(json.dumps(k, ensure_ascii=False))
                end = v
                ds = _index(v)
                k, v = next(ds, (None, None))
                if next(ds, None) is not None:
                    # > 1 different item
                    k = v = None
            prefix = u".".join(path) + u": "
            sub = line_diff(end, max_line_length, indent, min_equal_part, group_items)
            kind, level, line, ranges = next(sub)
            yield kind, level + indent, prefix + line, [Range(r.kind, r.start + len(prefix), r.end + len(prefix)) for r in ranges]
            for kind, level, line, ranges in sub:
                yield kind, level + indent, line, ranges
            if count > 1:
                yield Count, indent, u"{}".format(count), []
        yield Bracket, 0, close, []
    else:
        raise TypeError('invalid diff type {}'.format(type(diff)))


_TEXT_SYMS = {Add: u'+{}{}', Del: u'-{}{}', Mod: u'!{}{}', Bracket: u' {}{}', Count: u' {}(+ {} items with diffs affecting the same fields)'}
_HTML_TAGS = {Add: u'x-add', Del: u'x-del', Mod: u'x-mod', Bracket: u'x-bracket', Count: u'x-count'}
try:
    with open(os.path.join(os.path.dirname(__file__), 'jsondiff-template.html'), 'rb') as fd:
        _HTML_TEMPLATE = fd.read().decode('utf-8')
except Exception:
    _HTML_TEMPLATE = '<html><body>$CONTENT</body></html>'


def render_text(diffs, symbols=_TEXT_SYMS, **kwargs):
    '''Produce `diff`-style output. (It's not really valid, and cannot be used with `patch`.)

        :param diffs: iterable of `(name[, new_name], diff)` tuples, where `diff` is an object/line diff.
        :param symbols: a mapping of line kinds (see above) to symbols that are displayed
                        in the first column of each line of the diff.

    '''
    for i, entry in enumerate(diffs, 1):
        if len(entry) == 2:
            head, diff = entry
            yield u'{}: {}\n'.format(i, head)
            yield u'===================================================================\n'
        elif len(entry) == 3:
            a, b, diff = entry
            yield u'===================================================================\n'
            yield u'--- {}\n'.format(a)
            yield u'+++ {}\n'.format(b)
        else:
            raise ValueError('invalid diff, length {} (must be 2 or 3)\nentry: {}'.format(len(entry), entry))
        if diff is None or isinstance(diff, (Add, Del, Mod, dict, list)):
            diff = line_diff(diff, **kwargs)
        for kind, indent, line, ranges in diff:
            if kind is Mod and ranges:
                for kind in (Del, Add):
                    yield symbols[kind].format(u''.ljust(indent), u'')
                    last = 0
                    for range in ranges:
                        yield line[last:range.start]
                        if range.kind is kind:
                            yield line[range.start:range.end]
                        last = range.end
                    yield line[last:] + u'\n'
            elif kind in symbols:
                yield symbols[kind].format(u''.ljust(indent), line) + u'\n'


def render_html(diffs, template=_HTML_TEMPLATE, tags=_HTML_TAGS, **kwargs):
    '''Produce HTML5 output.

        :param diffs: iterable of `(name[, new_name], diff)` tuples, where `diff` is an object/line diff.
        :param template: a string in which the first occurrence of `$CONTENT` will be replaced
                         by a diff.
        :param tags: a mapping of line diff kinds (see above) to custom HTML5 tags.
                     Lines with kind not mapped to any tag are omitted from output.

    '''
    head, content, tail = template.partition('$CONTENT')
    yield head
    for i, entry in enumerate(diffs, 1):
        if len(entry) == 2:
            head, diff = entry
        elif len(entry) == 3:
            a, b, diff = entry
            head = u"{} → {}".format(a, b)
        else:
            raise ValueError('invalid diff')
        if diff is None or isinstance(diff, (Add, Del, Mod, dict, list)):
            diff = line_diff(diff, **kwargs)
        yield u'<h3><span class="muted">#{}:</span> {}</h3>'.format(i, head)
        yield u'<x-diff>'
        for kind, indent, line, ranges in diff:
            if kind not in tags:
                continue
            yield u'<{}>'.format(tags[kind]) + u''.ljust(indent)
            last = 0
            for range in ranges:
                yield xmlutils.escape(line[last:range.start])
                if range.kind in tags:
                    yield u'<{0}>{1}</{0}>'.format(tags[range.kind], xmlutils.escape(line[range.start:range.end]))
                if range.kind is not None:
                    last = range.end
            yield xmlutils.escape(line[last:])
            yield u'</{}>'.format(tags[kind])
        yield u'</x-diff>'
    yield tail


if __name__ == "__main__":
    import argparse

    def _no_group(diffs):
        return diffs

    def _group(diffs):
        for g in group(diffs):
            yield g[0]

    parser = argparse.ArgumentParser(description='Show differences between JSON files.')
    parser.add_argument('files', metavar='A B', type=argparse.FileType('r'), nargs='+', help='files to diff')
    parser.add_argument('--html', dest='renderer', action='store_const', const=render_html, default=render_text, help='render html5 output')
    parser.add_argument('--lines', action='store_true', help='treat input files as sequences of single-line JSON messages')
    parser.add_argument('--group', action='store_const', const=_group, default=_no_group, help='group diffs by similar affected fields, return the first diff in each group')
    parser.add_argument('--group-recursive', action='store_true', help='group object fields and array items by similar affected fields')
    args = parser.parse_args()

    if len(args.files) % 2:
        parser.error('odd number of files specified')

    def _read_file(fd):
        try:
            return json.load(fd)
        except Exception as err:
            parser.error("could not read {!r}: {}".format(fd, err))

    def _read_line(fd, i, text):
        try:
            return json.loads(text)
        except Exception as err:
            parser.error("could not read {!r}: {}".format(fd, err))

    try:
        _zip = itertools.zip_longest
    except AttributeError:
        _zip = itertools.izip_longest

    def _argv_diffs(files):
        it = iter(files)
        for old, new in zip(it, it):
            if args.lines:
                for i, (a, b) in enumerate(_zip(old, new, fillvalue='null'), 1):
                    x = _read_line(old, i, a)
                    y = _read_line(new, i, b)
                    d = diff(x, y)
                    if d is not None:
                        yield '{}#{}'.format(getattr(old, 'name', u'a'), i), '{}#{}'.format(getattr(new, 'name', u'a'), i), d
            else:
                x = _read_file(old)
                y = _read_file(new)
                d = diff(x, y)
                if d is not None:
                    yield getattr(old, 'name', u'a'), getattr(new, 'name', u'b'), d

    for r in args.renderer(args.group(_argv_diffs(args.files)), group_items=args.group_recursive):
        if sys.hexversion < 0x03000000:
            r = r.encode('utf-8')
        sys.stdout.write(r)
