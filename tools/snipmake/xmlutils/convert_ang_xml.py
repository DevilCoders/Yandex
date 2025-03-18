#!/usr/bin/env python
# -*- coding: utf-8 -*-

import codecs
import gzip
import optparse
import os
import re

import sys
ARCADIA_PATH = "../../.."
ANG_XML_PARSER_PATH = os.path.join(ARCADIA_PATH, "tools/snipmake/snipmetricsui/snipmetricsview")
SNIP_XML_PARSER_PATH = os.path.join(ARCADIA_PATH, "tools/snipmake/snippet_xml_parser/python")
SNIP_LEARNING_PATH = os.path.join(ARCADIA_PATH, "tools/snipmake/learning")
sys.path.append(SNIP_XML_PARSER_PATH)
sys.path.append(ANG_XML_PARSER_PATH)
sys.path.append(SNIP_LEARNING_PATH)

from learning_tools import TextUtils

import snips_parser
import snippet_dump_xml

XML_FORMATS = ["XmlDownloaderFormat", "XmlSerpFormat"]

class SimpleSnippet():
    def __init__(self):
        self.query = u""
        self.url = u""
        self.title = u""
        self.text = u""

    def ToUnicodeStr(self):
        result = self.query + u"\t"
        result += self.url + u"\t"
        result += self.title + u"\t"
        result += self.text
        return result

    def Parse(self, rawSnippet):
        if len(rawSnippet) == 6:
            # query, url, queryCharacterisics, queryExtraInfo, title, text = rawSnippet
            self.query = TextUtils.ToUnicode(rawSnippet[0])
            self.url = TextUtils.ToUnicode(rawSnippet[1])
            self.title = TextUtils.ClearText(rawSnippet[4])
            self.text = TextUtils.ClearText(rawSnippet[5])
        else:
            print >>sys.stderr, "Warning: uncorrect snippet tuple length:", len(rawSnippet), "instead of 6"
            for elem in rawSnippet:
                print >>sys.stderr, "\t%s" % elem


def IdentifyXmlFormat(filename):
    UNIQ_XML_DOWNLOADER_TAGS = "<(serpset|grabbed-serp-info|serp-components)"
    UNIQ_METRICS_TAGS = "<(searchresult|hostanswers)"

    if filename.endswith(".gz"):
        inputStream = gzip.open(filename, "r")
    else:
        inputStream = open(filename, "r")

    format = ""
    for line in inputStream:
        if re.match(UNIQ_XML_DOWNLOADER_TAGS, line) != None:
            format = XML_FORMATS[0]
            break
        if re.match(UNIQ_METRICS_TAGS, line) != None:
            format = XML_FORMATS[1]
            break
    inputStream.close()
    return format

def GetSnippetData(rawSnippet):
    simpleSnippet = SimpleSnippet()
    simpleSnippet.Parse(rawSnippet)

    queryDocPair = snippet_dump_xml.QDPairData()
    queryDocPair.region = "213"
    queryDocPair.relevance = "20"
    snippetData = snippet_dump_xml.SnippetData()

    queryDocPair.query = simpleSnippet.query
    queryDocPair.url = simpleSnippet.url
    snippetData.title = simpleSnippet.title
    if (len(snippetData.title) == 0):
        snippetData.title = "fake"
    
    fragment = snippet_dump_xml.FragmentData()
    fragment.text = simpleSnippet.text
    fragment.coords = "1 2"
    snippetData.fragments.append(fragment)

    return queryDocPair, snippetData

def WriteInXmlFormat(rawSnippets, filename):
    writer = snippet_dump_xml.SAXSnippetDumpXmlWriter(filename)

    writer.StartWrite()
    for rawSnippet in rawSnippets:
        queryDocPair, snippetData = GetSnippetData(rawSnippet)
        writer.AppendQDPairSnippets(queryDocPair, [snippetData])
    writer.EndWrite()

def WriteInTabSeparatedFormat(rawSnippets, filename):
    outputStream = codecs.open(filename, "w", "utf-8")
    for rawSnippet in rawSnippets:
        simpleSnippet = SimpleSnippet()
        simpleSnippet.Parse(rawSnippet)
        print >>outputStream, simpleSnippet.ToUnicodeStr()
    outputStream.close()

    
def callBack(poolName, qdPair, snippet):
    print "ssd"
    
if __name__ == '__main__':
    parser = optparse.OptionParser("usage: %prog [-t][-d|-m]  -i <input> -o <output>")
    parser.add_option("-i", "--input", action="store", dest="input", metavar="FILE",
                      help="input file name, gzipped files also supported", default = "")
    parser.add_option("-o", "--output", action="store", dest="output", metavar="FILE",
                      help="output file name", default = "")
    parser.add_option("-d", "--serp-downloader-format", action="store_true", dest="serp_downloader_format",
                      help="explicitly use serp downloader xml input file format, from http://serp-downloader.yandex-team.ru/")
    parser.add_option("-m", "--metrcis-file-format", action="store_true", dest="metrics_format",
                      help="explicitly use metrics xml input file format, from http://metrics.yandex-team.ru/import/serp")
    parser.add_option("-t", "--tab-separated-output", action="store_true", dest="tab_separated",
                      help="save output in old, plain tab separated format: <query> <url> <title> <snippet text>")
    (options, args) = parser.parse_args()

    if options.input == "" or options.output == "" or \
        (options.serp_downloader_format and options.metrics_format):
        parser.print_help()
        sys.exit(1)

    if options.serp_downloader_format:
        xmlInputFormat = XML_FORMATS[0]
    elif options.metrics_format:
        xmlInputFormat = XML_FORMATS[1]
    else:
        xmlInputFormat = IdentifyXmlFormat(options.input)

    if xmlInputFormat not in XML_FORMATS:
        print >>sys.stderr, "unsupported input file format"
        sys.exit(1)

    parser = snips_parser.SnippetsParser(options.input,
        snips_parser.SnippetsParser.ParserFileFormats[xmlInputFormat])
    rawSnippets = parser.ParseSnippets()

    if options.tab_separated:
        WriteInTabSeparatedFormat(rawSnippets, options.output)
    else:
        WriteInXmlFormat(rawSnippets, options.output)

