#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import xml.dom.minidom

from sys import argv
from xml.parsers import expat
from codecs import encode, decode

from xml.dom.minidom import Document

from xml.sax.saxutils import XMLGenerator
from xml.sax.xmlreader import AttributesNSImpl

# ------------- Consts -------------

# pool
POOL_TAG = u"pool"
POOL_ATTRIBUTES = [u"name"]

# qdpair
QUERY_DOC_PAIR_TAG = u"qdpair"
QUERY_DOC_PAIR_ATTRIBUTES = [u"region", u"richtree", u"url", u"relevance"]

# snippet
SNIPPET_ATTRIBUTES = [u"algorithm", u"rank", u"fragments", u"lines"]

SNIPPET_TAG = u"snippet"
SNIPPET_TITLE_TAG = u"title"
SNIPPET_FEATURES_TAG = u"features"

# fragments
SNIPPET_FRAGMENT_TAG = u"fragment"
SNIPPET_FRAGMENT_ATTRIBUTES = [u"coords"]
       
# marks
MARKS_TAG = u"marks"
MARK_TAG = u"mark"
MARK_ATTRIBUTES = [u"value", u"criteria", u"assessor", u"quality", u"timestamp"]
        
# comment
COMMENT_TAG = u"comment"
COMMENT_ATTRIBUTES = [u"assessor", u"tag"]

# ------------- Structures -------------
class PoolData:
    def __init__(self):
        self.name = ""
    
class QDPairData:
    def __init__(self):
        self.query = ""
        self.region = ""
        self.url = ""
        self.richtree = ""
        self.relevance = ""
        
class SnippetData:
    def __init__(self):
        self.algorithm = ""
        self.title = ""
        self.rank = ""
        self.featuresString = ""
        self.stringFragments = []
        self.marks = []
        self.comment = None
        fragments = ""
        lines = ""
        
class FragmentData:
    def __init__(self):
        self.coords = ""
        self.text = ""        

class MarkData:
    def __init__(self):
        self.value = ""
        self.criteria = ""
        self.assessor = ""
        self.quality = ""
        self.timestamp = ""
        
class CommentData:
    def __init__(self):
        self.assessor = ""
        self.tag = ""
        self.text = ""

# ------------- SAX Xml Writer -------------

class SAXSnippetDumpXmlWriter:
    def __init__(self, dataFile):
        self.output_file = open(dataFile, "w")
        self.xml_generator = XMLGenerator(self.output_file, "utf-8")
    
    def StartWrite(self):
        self.xml_generator.startDocument()
        self._startTag('pools')
        self._startTag('pool', {u'id': u"DWTest"})
        
    def EndWrite(self):
        self._endTag('pool')
        self._endTag('pools')
        
        self.xml_generator.endDocument()
        self.output_file.close()
    
    def AppendQDPairSnippets(self, qdPairData, snippetDataList):
        self._startTag(QUERY_DOC_PAIR_TAG, self._getAttributeValueDictFromData(qdPairData, QUERY_DOC_PAIR_ATTRIBUTES))
        
        if len(qdPairData.query) > 0:
            self._writeCDATA(qdPairData.query)
        
        for snippetData in snippetDataList:
            self._appendSnippetData(snippetData)
            
        self._endTag(QUERY_DOC_PAIR_TAG);
        
    def _appendSnippetData(self, snippetData):
        self._startTag(SNIPPET_TAG, self._getAttributeValueDictFromData(snippetData, SNIPPET_ATTRIBUTES))
        
        # title
        if len(snippetData.title) > 0:
            self._startTag(SNIPPET_TITLE_TAG)
            self._writeCDATA(snippetData.title)
            self._endTag(SNIPPET_TITLE_TAG)
    
        # fragments
        if len(snippetData.stringFragments) > 0:
            for fragmentData in snippetData.stringFragments:
                self._appendFragmentData(fragmentData)
    
        # featuresString
        if len(snippetData.featuresString) > 0:
            self._startTag(SNIPPET_FEATURES_TAG)
            self.xml_generator.characters(snippetData.featuresString)
            self._endTag(SNIPPET_FEATURES_TAG)
            
        # marks
        if len(snippetData.marks) > 0:
            self._startTag(MARKS_TAG)            
            for markData in snippetData.marks:
                self._appendMarkData(markData)                        
            self._endTag(MARKS_TAG)
        
        if not snippetData.comment == None:
            self._appendCommentData(snippetData.comment)
        
        self._endTag(SNIPPET_TAG);
                    
    def _appendFragmentData(self, fragmentData):
        self._startTag(SNIPPET_FRAGMENT_TAG, self._getAttributeValueDictFromData(fragmentData, SNIPPET_FRAGMENT_ATTRIBUTES))
        self._writeCDATA(fragmentData.text)
        self._endTag(SNIPPET_FRAGMENT_TAG)
        
    def _appendMarkData(self, markData):
        self._startTag(MARK_TAG, self._getAttributeValueDictFromData(markData, MARK_ATTRIBUTES))
        self._endTag(MARK_TAG)
        
    def _appendCommentData(self, commentData):
        self._startTag(COMMENT_TAG, self._getAttributeValueDictFromData(commentData, COMMENT_ATTRIBUTES))
        self._writeCDATA(commentData.text)
        self._endTag(COMMENT_TAG)
        
    # ------------ Base API ------------
    def _startTag(self, name, attr = {}, body = None, namespace = None):
        attr_vals = {}
        attr_keys = {}
        for key, val in attr.iteritems():
            key_tuple = (namespace, key)
            attr_vals[key_tuple] = val
            attr_keys[key_tuple] = key

        attr2 = AttributesNSImpl(attr_vals, attr_keys)
        self.xml_generator.startElementNS((namespace, name), name, attr2)
        if body:
            self.xml_generator.characters(body)
            
    def _endTag(self, name, namespace = None):
        self.xml_generator.endElementNS((namespace, name), name)
    
    def _writeCDATA(self, data):
        self.output_file.write("<![CDATA[")
        self.output_file.write(data.encode("utf-8"))
        self.output_file.write("]]>\n")
        
    def _getAttributeValueDictFromData(self, data, attributeNames):
        res_dict = {}
        for attrName in attributeNames:
            if attrName in data.__dict__ and len(data.__dict__[attrName]) > 0:
                res_dict[attrName] = data.__dict__[attrName]
        return res_dict
        
# ------------- Xml Writer -------------

class SnippetDumpXmlWriter:
    def __init__(self, dataFile, qdPairsDataDict, qdPairsHashCandidateListDict, processedQDPairs):
        self.dataFile = dataFile
        self.processedQDPairs = processedQDPairs
        self.qdPairsDataDict = qdPairsDataDict
        self.qdPairsHashCandidateListDict = qdPairsHashCandidateListDict
        self.doc = Document()
        
    def Write(self):
        poolsNode = self._createNode("pools", self.doc, {})
        poolNode = self._createNode("pool", poolsNode, {"name": "4q"})

        for qdPairsDataHash, processedSnipHashDict in self.processedQDPairs.iteritems():
            qdPairsNode = self.CreateQDPairsNode(poolNode, self.qdPairsDataDict[qdPairsDataHash])
            for snippetDataHash in processedSnipHashDict.keys():                
                self.CreateSnippetNode(qdPairsNode, self.qdPairsHashCandidateListDict[qdPairsDataHash][snippetDataHash])
                
        self._writeIntoFile()
        
    def CreateQDPairsNode(self, poolNode, qdPairsData):        
        newNode = self._createNode(QUERY_DOC_PAIR_TAG, poolNode, self._getAttributeValueDictFromData(qdPairsData, QUERY_DOC_PAIR_ATTRIBUTES))
        if len(qdPairsData.query) > 0:
            newNode.appendChild(self.doc.createCDATASection(qdPairsData.query))
        return newNode
    
    def CreateSnippetNode(self, qdPairsNode, snippetData):
        newSnippetNode = self._createNode(SNIPPET_TAG, qdPairsNode, self._getAttributeValueDictFromData(snippetData, SNIPPET_ATTRIBUTES))
        # title
        if len(snippetData.title) > 0:
            newTitleNode = self._createNode(SNIPPET_TITLE_TAG, newSnippetNode, {})
            newTitleNode.appendChild(self.doc.createCDATASection(snippetData.title))
        
        # fragments
        if len(snippetData.stringFragments) > 0:
            for fragmentData in snippetData.stringFragments:
                self.CreateFragmentNode(newSnippetNode, fragmentData)
        
        # featuresString
        if len(snippetData.featuresString) > 0:
            newFeatureNode = self._createNode(SNIPPET_FEATURES_TAG, newSnippetNode, {})
            newFeatureNode.appendChild(self.doc.createTextNode(snippetData.featuresString))
            
        # marks
        if len(snippetData.marks) > 0:
            marksNode = self._createNode(MARKS_TAG, newSnippetNode, {})
            for markData in snippetData.marks:
                self.CreateMarkNode(marksNode, markData)
                
        # comment
        if not snippetData.comment == None:
            self.CreateCommentNode(self, snippetNode, snippetData.comment)
                    
        return newSnippetNode
        
    def CreateFragmentNode(self, fragmentsNode, fragmentData):
        newFragmentNode = self._createNode(SNIPPET_FRAGMENT_TAG, fragmentsNode, self._getAttributeValueDictFromData(fragmentData, SNIPPET_FRAGMENT_ATTRIBUTES))
        newFragmentNode.appendChild(self.doc.createCDATASection(fragmentData.text))
        
        return newFragmentNode
        
    def CreateMarkNode(self, marksNode, markData):
        return self._createNode(MARK_TAG, marksNode, self._getAttributeValueDictFromData(markData, MARK_ATTRIBUTES))
    
    def CreateCommentNode(self, snippetNode, commentData):
        newCommentNode = self._createNode(COMMENT_TAG, snippetNode, self._getAttributeValueDictFromData(commentData, COMMENT_ATTRIBUTES))
        newSnippetNode.appendChild(self.doc.createCDATASection(commentData.text))        
        return newSnippetNode
    
    def _writeIntoFile(self):
        file = open(self.dataFile, "w")
        #file.write(self.doc.toxml(encoding = "utf8"))
        file.write(self.doc.toprettyxml(indent="\t", newl="\n", encoding = "utf-8"))
        file.close()
    
    def _getAttributeValueDictFromData(self, data, attributeNames):
        res_dict = {}
        for attrName in attributeNames:
            if attrName in data.__dict__ and len(data.__dict__[attrName]) > 0:
                res_dict[attrName] = data.__dict__[attrName]
        return res_dict
        
    def _createNode(self, nodeName, parentNode = None, attribs = {}):
        newNode = self.doc.createElement(nodeName)
        if not parentNode == None:
            parentNode.appendChild(newNode)
            
        for key, value in attribs.items():
            newNode.setAttribute(key, value)
        
        return newNode

# ------------- Xml Handler -------------
        
class SnippetDumpXmlReaderHandler:
    def __init__(self, snippetCallback = None, qdPairCallback = None):
        self.snippetCallback = snippetCallback
        self.qdPairCallback = qdPairCallback
        self.Reset()

    def Reset(self):
        self.state_stack = []
        self.currentPoolData = PoolData()
        self.currentQDPairData = QDPairData()
        self.currentSnippetData = SnippetData()
        self.currentSnippetDataList = []
        
        self.currentFragmentData = FragmentData()
        self.currentMarkData = MarkData()
        self.currentCommentData = CommentData()

    def GetTagNamesToSkipTo(self):
        return ['qdpair',]

    def StartElementHandler(self, name, attrs):
        self.state_stack.append(name)
        if name == POOL_TAG:
            self._copyAttributes(self.currentPoolData, attrs, POOL_ATTRIBUTES)        
        if name == QUERY_DOC_PAIR_TAG:
            self._copyAttributes(self.currentQDPairData, attrs, QUERY_DOC_PAIR_ATTRIBUTES)
        elif name == SNIPPET_TAG:
            self._copyAttributes(self.currentSnippetData, attrs, SNIPPET_ATTRIBUTES)
        elif name == SNIPPET_FRAGMENT_TAG:
            self._copyAttributes(self.currentFragmentData, attrs, SNIPPET_FRAGMENT_ATTRIBUTES)
        elif name == MARK_TAG:
            self._copyAttributes(self.currentMarkData, attrs, MARK_ATTRIBUTES)
        elif name == COMMENT_TAG:
            self._copyAttributes(self.currentCommentData, attrs, COMMENT_ATTRIBUTES)
            
    def EndElementHandler(self, name):
        self.state_stack.pop()
        
        if name == POOL_TAG:
            self.currentPoolData = PoolData()
        if name == QUERY_DOC_PAIR_TAG:
            if not self.qdPairCallback == None:
                self.qdPairCallback(self.currentPoolData.name, self.currentQDPairData, self.currentSnippetDataList)
            self.currentQDPairData = QDPairData()
            self.currentSnippetDataList = []
        elif name == SNIPPET_TAG:
            if not self.snippetCallback == None:
                self.snippetCallback(self.currentPoolData.name, self.currentQDPairData, self.currentSnippetData)
            self.currentSnippetDataList.append(self.currentSnippetData)
            self.currentSnippetData = SnippetData()
        elif name == SNIPPET_FRAGMENT_TAG:
            self.currentSnippetData.stringFragments.append(self.currentFragmentData)
            self.currentFragmentData = FragmentData()
        elif name == MARK_TAG:
            self.currentSnippetData.marks.append(self.currentMarkData)
            self.currentMarkData = MarkData()
        elif name == COMMENT_TAG:
            self.currentSnippetData.comment = self.currentCommentData;
            self.currentCommentData = CommentData()
            
    def CharacterDataHandler(self, data):
        lastTag = self.state_stack[len(self.state_stack) - 1]
        
        if len(data.strip()) == 0:
            return
        
        if lastTag == QUERY_DOC_PAIR_TAG:
            self.currentQDPairData.query += data
        elif lastTag == SNIPPET_TITLE_TAG:
            self.currentSnippetData.title += data
        elif lastTag == SNIPPET_FRAGMENT_TAG:
            self.currentFragmentData.text += data
        elif lastTag == SNIPPET_FEATURES_TAG:
            self.currentSnippetData.featuresString += data
        elif lastTag == COMMENT_TAG:
            self.currentCommentData.text += data
            
    def _copyAttributes(self, data, attributeDict, attributeNames):
        for attrName in attributeNames:
            if attrName in attributeDict:
                data.__dict__[attrName] = attributeDict[attrName]
        
# ------------- Xml Reader -------------
class SnippetDumpXmlReaderStopParseException(Exception):
    def __str__(self):
        return "Stop parse"
        
class SnippetDumpXmlReader:
    def __init__(self, dataFile, snippetCallback = None, qdPairCallback = None):
        self.dataFile = dataFile
        self.snippetCallback = snippetCallback
        self.qdPairCallback = qdPairCallback
        self.parser = self._createXmlParser()
        
    def _createXmlParser(self):
        parser = expat.ParserCreate()
        handler = SnippetDumpXmlReaderHandler(self.snippetCallback, self.qdPairCallback)
        
        parser.StartElementHandler = handler.StartElementHandler
        parser.EndElementHandler = handler.EndElementHandler
        parser.CharacterDataHandler = handler.CharacterDataHandler
        
        return parser
        
    def StartParse(self):
        self.parser.ParseFile(open(self.dataFile, "r"))
        
    # It's stupid way to stop parse but learner method getFactors!
    def StopParseByRaiseException(self):
        raise SnippetDumpXmlReaderStopParseException()
            
