#!/usr/bin/python
# -*- coding: utf-8 -*-

from xml.parsers import expat
from xml.sax.saxutils import XMLGenerator

def readPoint(filename):
    res = {}
    def start(tag, attrs):
        if len(attrs) == 3:
            res[attrs["system"]] = [attrs["value"], attrs["size"]]

    with open(filename, "r") as h:
        parser = expat.ParserCreate()
        parser.StartElementHandler = start
        parser.ParseFile(h)

    return res

class XmlWriter:
    def __init__(self, filename):
        self.out = open(filename, "w")
        self.gen = XMLGenerator(self.out, "utf-8")
        self.stack = []
    def startDocument(self):
        self.gen.startDocument()
    def endDocument(self):
        self.gen.endDocument()
        self.out.close()
    def startElement(self, name, attrs={}):
        self.stack.append(name)
        self.gen.startElement(name, attrs)
    def endElement(self):
        if len(self.stack):
            self.gen.endElement(self.stack.pop())
            self.eol()
    def text(self, value):
        self.gen.characters(value)
    def eol(self):
        self.gen.characters("\n")
