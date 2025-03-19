"""
Generator which counts amount of generated items
"""


class CountingGenerator(object):
    def __init__(self, generator):
        self.generator = generator
        self.count = 0

    def next(self):
        nxt = next(self.generator)
        self.count += 1
        return nxt

    def __iter__(self):
        return self

    __next__ = next  # python 3 compatibility
