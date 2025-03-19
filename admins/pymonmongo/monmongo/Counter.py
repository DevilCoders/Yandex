from collections import defaultdict
from termcolor import colored
from datetime import datetime


class MillisCounter:
    def __init__(self):
        self.count = 0
        self.millis = 0

    def inc(self, millis: int):
        self.count += 1
        self.millis += millis


TextCounter = lambda: defaultdict(MillisCounter)
NamespaceCounter = lambda: defaultdict(TextCounter)


def green(text):
    return colored(text, 'green', attrs=['bold'])


def blue(text):
    return colored(text, 'blue', attrs=['bold'])


def red(text):
    return colored(text, 'red', attrs=['bold'])


def yellow(text):
    return colored(text, 'yellow', attrs=['bold'])


class Counter:
    def __init__(self):
        # [dbname.collection] -> [text] -> [cnt, millis]
        self.data = NamespaceCounter()

    def inc(self, ns: str, op: str, text: str, millis: int):
        self.data[ns]['{} {}'.format(op, text)].inc(millis)

    def ns_count(self, ns: str):
        """ Calculate total count and millis for the namespace """
        count = 0
        millis = 0
        namespace = self.data[ns]
        for _, counter in namespace.items():
            count += counter.count
            millis += counter.millis
        return count, millis

    def dump(self, collections, queries):
        print(red('===== ' + datetime.now().ctime()))

        ns_weights = defaultdict(list)
        for ns in self.data:
            count, millis = self.ns_count(ns)
            ns_weights[ns] = [count, millis]

        ns_names = sorted(self.data, key=lambda x: -ns_weights[x][1] * 100000 - ns_weights[x][0])[:collections]
        for name in ns_names:
            if name == '':
                continue
            print(blue('{:50}'.format(name))
                  + yellow('{:5}'.format(ns_weights[name][0]))
                  + blue(' hits, ')
                  + yellow('{:5}'.format(ns_weights[name][1]))
                  + blue(' ms ')
                  )

            hits = self.data[name]
            hit_names = sorted(hits, key=lambda x: -hits[x].millis * 100000 - hits[x].count)[:queries]
            for hit in hit_names:
                print(yellow('{:5}'.format(hits[hit].count))
                      + green(' hits, ')
                      + yellow('{:5}'.format(hits[hit].millis))
                      + green(' ms  ')
                      + hit
                      )

            print()
        print()
