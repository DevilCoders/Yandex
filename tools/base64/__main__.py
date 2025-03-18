import sys
import argparse

import library.python.par_apply as lpa
import library.python.base64 as base64


def iter_stream(f, l):
    while True:
        buf = f.read(l)

        if buf:
            yield buf
        else:
            return


def join_parts(parts):
    if len(parts) == 1:
        return parts[0]

    return ''.join(parts)


def iter_chunked_stream(f, l):
    first_ch = f.read(l)

    if not first_ch:
        return

    def iter_chunks():
        yield first_ch

        for ch in iter_stream(f, l):
            yield ch

    if '\n' in first_ch:
        def iter_parts():
            for ch in iter_chunks():
                yield ch.replace('\n', '')

        parts = []
        parts_len = 0

        for part in iter_parts():
            parts.append(part)
            parts_len += len(part)

            while parts_len > l:
                cur = join_parts(parts)
                buf = cur[:l]
                lft = cur[l:]

                yield buf

                if lft:
                    parts = [lft]
                    parts_len = len(lft)
                else:
                    parts = []
                    parts_len = 0

        if parts:
            yield join_parts(parts)
    else:
        for x in iter_chunks():
            yield x


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('-d', '--decode', action='store_true')
    parser.add_argument('args', nargs='*')

    args = parser.parse_args()

    def iter_files():
        if args.args:
            for f in args.args:
                yield open(f, 'r')
        else:
            yield sys.stdin

    def iter_data(l):
        if args.decode:
            stream_func = iter_chunked_stream
        else:
            stream_func = iter_stream

        for f in iter_files():
            try:
                for ch in stream_func(f, l):
                    yield ch
            finally:
                f.close()

    if args.decode:
        func = base64.b64decode
    else:
        func = base64.b64encode

    for ch in lpa.par_apply(iter_data(3 * 1024 * 1024), func, 4):
        sys.stdout.write(ch)
