#!/usr/bin/env python
# -*- Encoding: windows-1251 -*-

"""Module for handling directindex_tool.
"""

from sys import stderr
from subprocess import Popen, PIPE

import os
import codecs

_PATH = os.path.join(".", "directindexer")
def setup(path):
    assert os.path.isfile(path)
    _PATH = path

# table for converting user-friendly commands to that which dindex understands
#  at the moment, commands coincide.
#  But they can be given some byte-value (in case if it increases efficiency).
_COMMANDS = [
    'InitServer',
    'CloseServer',
    'DocStart',
    'DocFinish',
    'Attributes',
    'ZoneStart',
    'ZoneFinish',
    'Text',
    'IncBreak',
    'NextWord',
    'Set'
]

class _CommandTransferer:
    def __init__(self, path, name, logName=None, attrsNames=[]):
        self.__pipe = Popen([path], stdin=PIPE)
        self.__input = codecs.getwriter("utf-8")(self.__pipe.stdin, "xmlcharrefreplace")

        self.__log = None
        if logName is not None:
            self.__log = codecs.getwriter("utf-8")(file(logName, 'wb'), "xmlcharrefreplace")

        self.send('InitServer', [name, "index-", "20", ",".join(attrsNames)])
        self.__isOpen = True

    def __del__(self):
        if self.__isOpen:
            self.send('CloseServer')

    def closeAndWait(self):
        if self.__isOpen:
            self.__isOpen = False
            self.send('CloseServer')
            return self.__pipe.wait()
        return 0

    def __stringify(self, *sequence):
        return '\t'.join(map(lambda x: x.replace('\t', ' ').replace('\n', ' '), sequence)) + "\n"

    def send(self, command, data=[]):
        assert command in _COMMANDS
        out = self.__stringify(command, *data)
        self.__input.write(out)
        if self.__log:
            self.__log.write(out)

        if command == 'CloseServer':
            self.__input.flush()

class DirectIndex:
    def __init__(self, name, logName=None, attrsNames=[]):
        self.__ct = _CommandTransferer(_PATH, name, logName, attrsNames)
        self.__send = self.__ct.send

    def CloseAndWait(self):
        """Closes indexing: waits the procsess to end;
        returns return code.
        """
        return self.__ct.closeAndWait()

    def BeginDocument(self):
        self.__send('DocStart')

    def EndDocument(self):
        self.__send('DocFinish')

    def AddAttributes(self, attrs):
        """Add attributes (sequence of pairs: (name, value)) to current document."""
        seq = []
        for nameValuePair in attrs:
            seq.extend(nameValuePair)
        self.__send('Attributes', seq)

    def BeginZone(self, name, attrs=None):
        seq = [name]
        if attrs:
            for nameValuePair in attrs:
                seq.extend(nameValuePair)
        self.__send('ZoneStart', seq)

    def EndZone(self, name):
        assert isinstance(name, basestring)
        self.__send('ZoneFinish', [name])

    def AddText(self, text, attrs=None):
        assert isinstance(text, basestring)

        seq = [text]
        if attrs:
            for nameValuePair in attrs:
                seq.extend(nameValuePair)
        if text:
            self.__send('Text', seq)

    def IncBreak(self):
        """Increment sentence position."""
        self.__send('IncBreak')

    def NextWord(self):
        """Increment word position."""
        self.__send('NextWord')

    def Set(self, command, value):
        self.__send("Set", [command, str(value)])
