#!/usr/bin/env python

from sys import stdin, stdout
from re import compile

class Patterns:
    text, line = compile(r'\bText #(\d+): {(.*)}$'), compile(r'(.*)#(\d+)$')
    all = (text, line)

    @staticmethod
    def match(line):
        for pattern in Patterns.all:
            match = pattern.match(line)
            if match:
                return (match, pattern)
        return (None, None)

text = None
for line in stdin:
    match, pattern = Patterns.match(line)
    if pattern is Patterns.text:
        text = match.groups()
        continue
    if pattern is Patterns.line:
        if match.groups()[1] != text[0]:
            raise RuntimeError('the text number check failed')
        line = '%s{%s}' % (match.groups()[0], text[1])
        line = '%s\n' % line[0:1023]
    stdout.write(line)
