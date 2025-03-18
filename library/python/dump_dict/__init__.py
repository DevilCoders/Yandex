from six import iteritems

bi = '\t'


def compact(it):
    prev = None

    for i in it:
        if prev and prev.strip() == '}' and i.strip() == '{':
            prev = prev + ' {'
        else:
            if prev:
                yield prev

            prev = i

    if prev:
        yield prev


def dumps(x):
    if isinstance(x, list):
        def do_iter():
            yield '['

            for y in x:
                for k in dumps(y):
                    yield bi + k

            yield ']'

        for z in compact(do_iter()):
            yield z
    elif isinstance(x, dict):
        yield '{'

        for k, v in sorted(iteritems(x), key=lambda x: str(x[0])):
            ss = list(dumps(v))

            if ss:
                yield bi + str(k) + ' = ' + ss[0]

                for z in ss[1:]:
                    yield bi + z
            else:
                yield bi + str(k)

        yield '}'
    else:
        try:
            yield str(x)
        except UnicodeEncodeError:
            yield x.encode('utf-8')


def dump(x):
    return '\n'.join(dumps(x))
