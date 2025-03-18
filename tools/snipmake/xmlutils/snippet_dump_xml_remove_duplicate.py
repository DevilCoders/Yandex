#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import random

sys.path.append("../learning/")
sys.path.append("../snippet_xml_parser/python")

from sys import argv
from optparse import OptionParser
from snippet_dump_xml import SnippetDumpXmlWriter
from snippet_dump_xml import SAXSnippetDumpXmlWriter
from snippet_dump_xml import SnippetDumpXmlReader
from snippet_dump_xml import QDPairData
from snippet_dump_xml import SnippetData
from snippet_dump_xml import FragmentData
from snippet_dump_xml import MarkData
from candidate_tools import CandidateUpdaterBase
from candidate_tools import CandidateUpdaterByText

from candidate_parser import SnippetsData

class ProcessCandidateCallback:
    def __init__(self, xmlWriter):
        self.xmlWriter = xmlWriter
        self.xmlWriter.StartWrite()
        
    def __call__(self, poolID, qdPairData, snippetDataList):
        qdSnipTextDict = {}
        for snippetData in snippetDataList:
            qdSnipTextDict[CandidateUpdaterByText.GetSnippetText(snippetData)] = snippetData
        
        if qdPairData.url.count("://") == 0:
            qdPairData.url = "http://" + qdPairData.url
        
        self.xmlWriter.AppendQDPairSnippets(qdPairData, qdSnipTextDict.values())
        
    def __del__(self):
        self.xmlWriter.EndWrite()
        
def RemoveDuplicate(old_path, new_path):
    xmlWriter = SAXSnippetDumpXmlWriter(new_path)

    dumpXmlParser = SnippetDumpXmlReader(old_path, None, ProcessCandidateCallback(xmlWriter))
    dumpXmlParser.StartParse()
    
if __name__=="__main__":
    usage = '''usage: %prog <old> <new>'''
    parser = OptionParser(usage)
    
    parser.add_option("-o", "--old", action="store", dest="old", metavar="PATH",
                      help="old format file name")
    parser.add_option("-n", "--new", action="store", dest="new", metavar="PATH",
                      help="new format file name")
    
    (options, args) = parser.parse_args()
    RemoveDuplicate(options.old, options.new)
    
