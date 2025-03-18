#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys

sys.path.append("../learning/")
sys.path.append("../snippet_xml_parser/python")

from sys import argv
from optparse import OptionParser
from snippet_dump_xml import SnippetDumpXmlWriter
from snippet_dump_xml import SAXSnippetDumpXmlWriter
from snippet_dump_xml import QDPairData
from snippet_dump_xml import SnippetData
from snippet_dump_xml import FragmentData
from snippet_dump_xml import MarkData
from candidate_tools import CandidateUpdaterBase
from candidate_parser import SnippetsData

def ConvertFromPlainFormatIntoXmlWitPlainGrouping(old_path, new_path):
    lastQDPairHash = None
    lastQDPairData = None
    currentSnippetDataList = []
    
    writer = SAXSnippetDumpXmlWriter(new_path)
    writer.StartWrite()
    
    with open(old_path) as datafile:
      for line in datafile:
          candidate = SnippetsData(line)
          
          qdPairData = QDPairData()
          qdPairData.query = candidate.query.decode("utf8")
          qdPairData.region = candidate.region
          qdPairData.url = candidate.url
          qdPairData.relevance = candidate.relevance
          
          snippetData = SnippetData()
          snippetData.algorithm = candidate.algorithm
          snippetData.title = candidate.title.decode("utf8")
          snippetData.featuresString = candidate.sfeatures
          snippetData.rank = candidate.rank
          
          fragmentData = FragmentData()
          fragmentData.coords = candidate.scoords
          fragmentData.text = candidate.snippet.decode("utf8")                    
          snippetData.fragments.append(fragmentData)
                    
          for name, value in candidate.marks.iteritems():
            mark = MarkData()
            mark.value = str(value)
            mark.criteria = name
            snippetData.marks.append(mark)          
          
          qdPairHash = CandidateUpdaterBase.GetQDPairHash(qdPairData)
          
          if not lastQDPairHash == qdPairHash and not lastQDPairHash == None:
              writer.AppendQDPairSnippets(lastQDPairData, currentSnippetDataList)
              lastQDPairHash = qdPairHash
              currentSnippetDataList = [snippetData]
          else:
              currentSnippetDataList.append(snippetData)
          lastQDPairData = qdPairData
          lastQDPairHash = qdPairHash
          
    writer.AppendQDPairSnippets(lastQDPairData, currentSnippetDataList)
    writer.EndWrite()

def ConvertFromPlainFormatIntoXml(old_path, new_path):
    processedQDPairs = {}
    qdPairsDataDict = {}
    qdPairsHashCandidateListDict = {}
    
    snipCount = 0
    with open(old_path) as datafile:
      for line in datafile:
          candidate = SnippetsData(line)
          
          qdPairData = QDPairData()
          qdPairData.query = candidate.query.decode("utf8")
          qdPairData.region = candidate.region
          qdPairData.url = candidate.url
          qdPairData.relevance = candidate.relevance
          
          
          qdPairHash = CandidateUpdaterBase.GetQDPairHash(qdPairData)
          if not qdPairHash in qdPairsDataDict:
              qdPairsDataDict[qdPairHash] = qdPairData
              qdPairsHashCandidateListDict[qdPairHash] = {}
              processedQDPairs[qdPairHash] = {}
          
          snippetData = SnippetData()
          snippetData.algorithm = candidate.algorithm
          snippetData.title = candidate.title.decode("utf8")
          snippetData.featuresString = candidate.sfeatures
          snippetData.rank = candidate.rank
          
          fragmentData = FragmentData()
          fragmentData.coords = candidate.scoords
          fragmentData.text = candidate.snippet.decode("utf8")                    
          snippetData.fragments.append(fragmentData)
                    
          for name, value in candidate.marks.iteritems():
            mark = MarkData()
            mark.value = str(value)
            mark.criteria = name
            snippetData.marks.append(mark)
          
          snipHash = repr(snipCount)
                
          if not snipHash in qdPairsHashCandidateListDict[qdPairHash]:
              qdPairsHashCandidateListDict[qdPairHash][snipHash] = snippetData
              processedQDPairs[qdPairHash][snipHash] = None
              
          snipCount +=1
              
    writer = SnippetDumpXmlWriter(new_path, qdPairsDataDict, qdPairsHashCandidateListDict, processedQDPairs)
    writer.Write()

    
if __name__=="__main__":
    usage = '''usage: %prog -o <old> -n <new>'''
    parser = OptionParser(usage)
    
    parser.add_option("-o", "--old", action="store", dest="old", metavar="PATH",
                      help="old format file name")
    parser.add_option("-n", "--new", action="store", dest="new", metavar="PATH",
                      help="new format file name")
    
    (options, args) = parser.parse_args()
    ConvertFromPlainFormatIntoXmlWitPlainGrouping(options.old, options.new)
    
