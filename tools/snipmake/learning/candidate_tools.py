#!/usr/local/bin/python
# -*- coding: utf8 -*-

import hashlib
import unicodedata
import re, urllib, urlparse, cgi, bisect

from candidate_parser import SnippetsData
from codecs import encode, decode
from htmlentitydefs import name2codepoint
from sys import argv, stderr, path
path.append("../snippet_xml_parser/python")
from snippet_dump_xml import SnippetDumpXmlReader
from snippet_dump_xml import SnippetDumpXmlWriter
from urllib import quote, unquote

########################### Utils ############################################
class CandidateUpdaterUtils:
    __path_reserved_symbols = "$-_.+!*'(),~;/:@=&"

    @staticmethod
    def HtmlEntityDecodeFull(text):
        if len(text) == 0:
            return ""
        cur = 0
        while True:
            curText = CandidateUpdaterUtils.HtmlEntityDecode(text)
            if curText == text:
                return curText
            text = curText
            cur += 1
            if cur > 5:
                return curText

    @staticmethod
    def HtmlEntityDecode(text):
        try:
            return re.sub('&(%s);' % '|'.join(name2codepoint), lambda m: chr(name2codepoint[m.group(1)]), text)
        except:
            return text
    
    @staticmethod
    def RemoveProto(url, only_http=False):
        return re.sub("^http://" if only_http else "^\w+://", "", url)
    
    @staticmethod
    def UrlNormalize(url, without_www=True, remove_final_slash=True):
        if len(url) == 0:
            return ""
        try:
            _u = urllib.unquote(url)
            _u_parts = list(urlparse.urlsplit(_u))
            if _u_parts[0] == '':
                _u_parts = list(urlparse.urlsplit("http://" + _u))
            _u_parts[1] = _u_parts[1].lower()
            if without_www:
                _u_parts[1] = re.sub("^www\.([\w-]+\.)", lambda sres: sres.group(1), _u_parts[1])
            _u_parts[2] = re.sub("/+$", "/", urllib.quote(_u_parts[2], CandidateUpdaterUtils.__path_reserved_symbols))
            if _u_parts[3] != '':
                if "=" in _u_parts[3]:
                    _attrs = cgi.parse_qs(_u_parts[3], keep_blank_values=True)
                    attrs = []
                    for k, l_v in sorted(_attrs.iteritems()):
                        for v in l_v:
                            attrs.append((k, v))
                    _u_parts[3] = urllib.urlencode(attrs)
                else:
                    _u_parts[3] = urllib.quote(_u_parts[3], CandidateUpdaterUtils.__path_reserved_symbols)
            if not _u_parts[3] and not _u_parts[4] and (_u_parts[2] == "/" or remove_final_slash and _u_parts[2].endswith("/")):
                _u_parts[2] = _u_parts[2][:-1]
            return CandidateUpdaterUtils.RemoveProto(urlparse.urlunsplit(_u_parts), only_http=True)
        except:
            return url

######################################### Main ############################################    
class CandidateUpdaterBase:
    def __init__(self, oldCandidateFileName, newCandidateDumpFileName, newCandidateFileName):
        self.oldCandidateFileName = oldCandidateFileName
        self.newCandidateDumpFileName = newCandidateDumpFileName
        self.newCandidateFileName = newCandidateFileName
        
        self.qdPairsDataDict = {}
        self.qdPairsHashCandidateListDict = {}
        self.processedQDPairsHash = {}
        
        self.oldCandidateCount = 0
        self.updatedCandidateCount = 0
        
    def ProcessOldCandidateCallback(self, poolID, qdPairData, snippetData):        
        snipHash = self.GetSnippetHash(qdPairData, snippetData)
        qdPairHash = self.GetQDPairHash(qdPairData)
        if not qdPairHash in self.qdPairsDataDict:
            self.qdPairsDataDict[qdPairHash] = qdPairData
            self.qdPairsHashCandidateListDict[qdPairHash] = {}
                
        if not snipHash in self.qdPairsHashCandidateListDict[qdPairHash]:
            self.qdPairsHashCandidateListDict[qdPairHash][snipHash] = snippetData
            self.oldCandidateCount += 1
    
    def ProcessNewCandidateCallback(self, poolID, qdPairData, snippetData):
        candidateHash = self.GetSnippetHash(qdPairData, snippetData)
        qdPairHash = CandidateUpdaterBase.GetQDPairHash(qdPairData)
        
        if (qdPairHash in self.qdPairsHashCandidateListDict) and (candidateHash in self.qdPairsHashCandidateListDict[qdPairHash]):
            if not qdPairHash in self.processedQDPairsHash:                
                self.processedQDPairsHash[qdPairHash] = {}

            if not candidateHash in self.processedQDPairsHash[qdPairHash]:
                self.updatedCandidateCount += 1
                
            self.processedQDPairsHash[qdPairHash][candidateHash] = None
            self.qdPairsHashCandidateListDict[qdPairHash][candidateHash].featuresString = snippetData.featuresString
    
    def GetSnippetID(self, snippetData):
        raise NotImplementedError( "Should have implemented this" )
        
    @staticmethod
    def GetQDPairHash(qdPairData):
        return hashlib.sha224("_".join([CandidateUpdaterUtils.HtmlEntityDecodeFull(qdPairData.query.encode("utf8")), CandidateUpdaterUtils.UrlNormalize(qdPairData.url.encode("utf8")), qdPairData.region.encode("utf8")])).hexdigest()
        
    def GetSnippetHash(self, qdPairData, snippetData):
        return hashlib.sha224("_".join([CandidateUpdaterUtils.HtmlEntityDecodeFull(qdPairData.query.encode("utf8")), CandidateUpdaterUtils.UrlNormalize(qdPairData.url.encode("utf8")), qdPairData.region.encode("utf8"), self.GetSnippetID(snippetData)])).hexdigest()
    
    def GetLostSnippetAlgoDict(self):
        lost_snip = 0
        for qdPairsDataHash, snipHashDict in self.qdPairsHashCandidateListDict.iteritems():
            dif_set = set(snipHashDict.keys())
            if qdPairsDataHash in self.processedQDPairsHash:
                processedSnipHashSet = set(self.processedQDPairsHash[qdPairsDataHash].keys())
                snipHashSet = set(snipHashDict.keys())
                dif_set = snipHashSet - processedSnipHashSet
                
            lost_snip += len(dif_set)
        return lost_snip
    
    def Update(self):
        '''
        update candidates from <oldCandidateFileName> with features from <newCandidateDumpFileName>
        store result in <newCandidateFileName>
        '''
        
        print "Parse old cands: ", self.oldCandidateFileName
        dumpXmlParser = SnippetDumpXmlReader(self.oldCandidateFileName, self.ProcessOldCandidateCallback)
        dumpXmlParser.StartParse()
        
        print "Parse new cands: ", self.newCandidateDumpFileName        
        dumpXmlParser = SnippetDumpXmlReader(self.newCandidateDumpFileName, self.ProcessNewCandidateCallback)
        dumpXmlParser.StartParse()
        
        writer = SnippetDumpXmlWriter(self.newCandidateFileName, self.qdPairsDataDict, self.qdPairsHashCandidateListDict, self.processedQDPairsHash)
        writer.Write()
                            
        print 'tasks: ', len(self.qdPairsDataDict.keys())
        print 'cands: ', self.oldCandidateCount
        print 'updated tasks: ', self.updatedCandidateCount
        print 'lost candidates: ', self.GetLostSnippetAlgoDict()  
    
class CandidateUpdaterByText(CandidateUpdaterBase):
    def GetSnippetID(self, snipData):
        return CandidateUpdaterUtils.HtmlEntityDecodeFull(CandidateUpdaterByText.GetSnippetText(snipData).encode("utf8"))
    
    @staticmethod
    def GetSnippetText(snipData):
        snip_fragment_texts = []
        for fragment in snipData.fragments:
            snip_fragment_texts.append(fragment.text.replace("...", "").strip())
        return " ".join(snip_fragment_texts)
        
class CandidateUpdaterByCoordinates(CandidateUpdaterBase):
    def GetSnippetID(self, snipData):
        return self.GetSnippetCoords(snipData).encode("utf8")
        
    def GetSnippetCoords(self, snipData):
        coords = []
        for fragment in snipData.fragments:
            coords.append(fragment.coords.strip())
        
        return " ".join(coords)
        
#################################################################################################################################
    
INVERTED = True
def mergeCandidates(electfilename, resfilename = None):
    '''merge candidates from <electfilename> to have an integrate assessor score'''
    alldata = {}
    allmarks = {}
    with open(electfilename) as infile:
        for line in infile:
            snippet = SnippetsData(line)
            common = snippet.GetSourceString("candidate")
            if common not in alldata:
                alldata[common] = {'query_info':[], 'readability':[], 'content':[]}
            alldata[common][snippet.marks.keys()[0]].append([ snippet.assessor, float(snippet.marks.values()[0]) ])
            markkey = (snippet.id, snippet.marks.keys()[0])
            if markkey not in allmarks:
                allmarks[markkey] = []
            allmarks[markkey].append([ snippet.assessor, float(snippet.marks.values()[0]) ])
    print len(alldata), 'results\n'
    
    with open(resfilename if resfilename else electfilename + '_combined', "w") as resfile:
        for common, marks in sorted(alldata.iteritems()):
            combined = []
            assessors = []
            for criteria, all in marks.iteritems():
                if len(all)==0:
                    continue
                if INVERTED:
                    allmark = allmarks[(common.split('\t')[0], criteria)]
                    combined.append([ criteria, str(max(mar[1] for mar in allmark) +1- sum([mark[1] for mark in all]) / float(len(all))) ])
                else:
                    combined.append([ criteria, str(sum([mark[1] for mark in all]) / float(len(all))) ])
                assessors.extend([mark[0]+':'+criteria for mark in all])
            print >>resfile, '\t'.join([common, ' '.join([':'.join(mark) for mark in combined]), ' '.join(assessors), '0'])

# TODO!
def reScore(electfilename, resfilename = None):
    '''move assessment scores to [0,1] scale'''
    alldata = {}
    with open(electfilename) as infile:
        for line in infile:
            snippet = SnippetsData(line)
            candidate = snippet.GetSourceString("candidate")
            if snippet.id not in alldata:
                alldata[snippet.id] = {}
            criteria, mark = snippet.marks.items()[0]
            if snippet.assessor not in alldata[snippet.id]:
                alldata[snippet.id][snippet.assessor] = {'query_info':[], 'readability':[], 'content':[]}
            alldata[snippet.id][snippet.assessor][criteria].append([candidate, mark])
    
    with open(resfilename if resfilename else electfilename + '_rescored', "w") as resfile:
        for id, assessors in sorted(alldata.iteritems()):
            for assessor, allmarks in assessors.iteritems():
                for criteria, marks in allmarks.iteritems():
                    if len(marks)==0:
                        continue
                    groups = max([mark[1] for mark in marks])
                    for mark in marks:
                        mark[1] = (mark[1] - 0.5) / groups
                        if INVERTED:
                            mark[1] = 1 - mark[1]
                        print >>resfile, '\t'.join([mark[0], criteria+':'+str(mark[1]), assessor, '0'])    

if __name__ == "__main__":
    parser = CandidateUpdaterByCoordinates("old.xml", "dump.xml", "res.xml")
    parser.Update()
    