#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
from sys import argv, path
from options import get_option_value
path.append(get_option_value("snippetXmlFormatParser"))
import snippet_dump_xml
from xml.parsers import expat
import gzip
from codecs import encode
from log import writeToLog


class XmlDownloaderFormatParsingHandler:
    def __init__(self, collectSnippets=False, dataFilePath="",
                 dataFile=None, taskId=-1, progressCallback=None):
        self.collectSnippets = collectSnippets
        self.queryTagName = u"text"
        self.docTagName = u"serp-component"
        self.requiredDocAttributes = {u"type": u"SEARCH_RESULT"}
        self.urlTagName = u"page-url"
        self.titleTagName = u"title"
        self.snippetTagName = u"snippet"
        self.sitelinkTagName = u"sitelink"
        self.taskId = taskId
        self.progressCallback = progressCallback
        self.dataFilePath = dataFilePath
        self.dataFile = dataFile
        self.parsedData = []
        self.counter = 0
        self.Reset()

    def Reset(self):
        self.currentUrl = ""
        self.currentTitle = ""
        self.currentSnippet = ""
        self.docStartFound = False
        self.urlStartFound = False
        self.titleStartFound = False
        self.snippetStartFound = False
        self.queryStartFound = False
        self.sitelinkStartFound = False

    def StartElementHandler(self, name, attrs):
        if name == self.queryTagName:
            self.queryStartFound = True
            self.currentQuery = ""
        elif name == self.docTagName:
            self.docStartFound = True
            for requiredAttr in self.requiredDocAttributes.iterkeys():
                if (requiredAttr not in attrs) or (attrs[requiredAttr] != self.requiredDocAttributes[requiredAttr]):
                    self.docStartFound = False
        elif self.docStartFound:
            if name == self.sitelinkTagName:
                self.sitelinkStartFound = True
            if not self.sitelinkStartFound:
                if name == self.urlTagName:
                    self.urlStartFound = True
                elif self.collectSnippets:
                    if name == self.titleTagName:
                        self.titleStartFound = True
                    elif name == self.snippetTagName:
                        self.snippetStartFound = True

    def GetTagNamesToSkipTo(self):
        return [self.queryTagName, self.docTagName]

    def EndElementHandler(self, name):
        if name == self.docTagName:
            if self.docStartFound:
                self.counter += 1
                if self.progressCallback and self.counter % 100 == 0:
                    self.progressCallback(self.taskId, 100*self.dataFile.tell() / float(os.path.getsize(self.dataFilePath)))

                if self.currentUrl.strip() != "":
                    self.parsedData.append((self.currentQuery, self.currentUrl,
                                            self.currentTitle, self.currentSnippet))
            self.Reset()
        elif name == self.urlTagName:
            self.urlStartFound = False
        elif name == self.titleTagName:
            self.titleStartFound = False
        elif name == self.snippetTagName:
            self.snippetStartFound = False
        elif name == self.queryTagName:
            self.queryStartFound = False

    def CharacterDataHandler(self, data):
        data = encode(data, "utf8")
        if self.urlStartFound:
            self.currentUrl += data
        elif self.titleStartFound:
            self.currentTitle += data
        elif self.snippetStartFound:
            self.currentSnippet += data
        elif self.queryStartFound:
            self.currentQuery += data


class XmlSerpFormatParsingHandler:
    def __init__(self, collectSnippets = False, dataFilePath = "", dataFile = None, taskId = -1, progressCallback = None):
        self.collectSnippets = collectSnippets
        self.queryTagName = u"query"
        self.queryAttributeName = u"text"
        self.urlTagName = [u"pageurl", u"page-url"]
        self.titleTagName = u"title"
        self.snippetTagName = u"snippet"
        self.docTagName = u"serp-component"
        self.queryCharacteristicsAttribute = u"characteristics"
        self.queryExtraInfoAttributeName = "extrainfo"
        self.requiredDocAttributes = {u"type":u"SEARCH_RESULT"}
        self.taskId = taskId
        self.progressCallback = progressCallback
        self.dataFilePath = dataFilePath
        self.dataFile = dataFile
        self.counter = 0

        self.Reset()
        self.parsedData = []

    def StartElementHandler(self, name, attrs):
        if name == self.queryTagName:
            self.currentQuery = attrs[self.queryAttributeName].strip()
            if self.queryCharacteristicsAttribute in attrs.iterkeys():
                self.queryCharacteristics = attrs[self.queryCharacteristics].strip().split(";")
            if self.queryExtraInfoAttributeName in attrs.iterkeys():
                attrVal = attrs[self.queryExtraInfoAttributeName].strip()
                self.queryExtraInfo = SnippetsParser.parseQueryExtraInfoDict(attrVal)

        elif name == self.docTagName:
            self.docStartFound = True
            for requiredAttr in self.requiredDocAttributes.iterkeys():
                if (requiredAttr not in attrs) or (attrs[requiredAttr] != self.requiredDocAttributes[requiredAttr]):
                    self.docStartFound = False
        elif self.docStartFound:
            if name in self.urlTagName:
                self.urlStartFound = True
            elif self.collectSnippets:
                if name == self.titleTagName:
                    self.titleStartFound = True
                elif name == self.snippetTagName:
                    self.snippetStartFound = True

    def Reset(self):
        self.currentQuery = ""
        self.currentUrl = ""
        self.currentTitle = ""
        self.currentSnippet = ""
        self.queryCharacteristics = ""
        self.queryCharacteristics = []
        self.queryExtraInfo = {}
        self.docStartFound = False
        self.urlStartFound = False
        self.titleStartFound = False
        self.snippetStartFound = False
        self.counter = 0

    def GetTagNamesToSkipTo(self):
        return [self.queryTagName, self.docTagName]

    def EndElementHandler(self, name):
        if name == self.docTagName:
            if self.docStartFound:
                self.counter += 1
                if self.progressCallback and self.counter % 100 == 0:
                    self.progressCallback(self.taskId, 100.0*self.dataFile.tell() / float(os.path.getsize(self.dataFilePath)))
                if self.currentUrl.strip() != "":
                    if self.collectSnippets:
                        self.parsedData.append((self.currentQuery, self.currentUrl, self.queryCharacteristics, self.queryExtraInfo, self.currentTitle, self.currentSnippet))
                    else:
                        self.parsedData.append((self.currentQuery, self.currentUrl, self.queryCharacteristics, self.queryExtraInfo))
            self.docStartFound = False
            self.urlStartFound = False
            self.titleStartFound = False
            self.snippetStartFound = False
            self.currentUrl = ""
            self.currentTitle = ""
            self.currentSnippet = ""
        elif name in self.urlTagName:
            self.urlStartFound = False
        elif name == self.titleTagName:
            self.titleStartFound = False
        elif name == self.snippetTagName:
            self.snippetStartFound = False

    def CharacterDataHandler(self, data):
        data = data
        if self.urlStartFound:
            self.currentUrl += data
        elif self.titleStartFound:
            self.currentTitle += data
        elif self.snippetStartFound:
            self.currentSnippet += data

class XmlSnippetsFormatParsingHandler:
    def __init__(self, collectSnippets = False, dataFilePath = "", dataFile = None, taskId = -1, progressCallback = None):
        self.collectSnippets = collectSnippets
        self.parsedData = []
        self.Reset()
        self.snippetTagName = "snippet"
        self.queryTagName = "query"
        self.titleTagName = "title"
        self.urlTagName = "url"
        self.headlineTagName = "headline"
        self.snippetTextTagName = "text"
        self.queryCharacteristicsAttributeName = "characteristics"
        self.queryExtraInfoAttributeName = "extrainfo"
        self.queryCharacteristics = ""
        self.taskId = taskId
        self.progressCallback = progressCallback
        self.dataFilePath = dataFilePath
        self.dataFile = dataFile
        self.counter = 0

    def StartElementHandler(self, name, attrs):
        if name == self.snippetTagName:
            self.snippetBeginFound = True
            return

        if self.snippetBeginFound:
            if name == self.queryTagName:
                self.queryBeginFound = True
                if self.queryCharacteristicsAttributeName in attrs.iterkeys():
                    self.queryCharacteristics = attrs[self.queryCharacteristicsAttributeName].strip().split(";")
                if self.queryExtraInfoAttributeName in attrs.iterkeys():
                    attrVal = attrs[self.queryQueryExtraInfoAttributeName].strip()
                    self.queryExtraInfo = SnippetsParser.parseQueryExtraInfoDict(attrVal)
            elif name == self.titleTagName:
                self.titleBeginFound = True
            elif name == self.headlineTagName:
                self.headlineBeginFound = True
            elif name == self.snippetTextTagName:
                self.snippetTextBeginFound = True
            elif name == self.urlTagName:
                self.urlBeginFound = True
    def Reset(self):
        self.currentQuery = ""
        self.currentUrl = ""
        self.currentTitle = ""
        self.currentSnippetText = ""
        self.currentHeadline = ""
        self.queryCharacteristics = []
        self.queryExtraInfo = {}
        self.snippetBeginFound = False
        self.queryBeginFound = False
        self.titleBeginFound = False
        self.urlBeginFound = False
        self.headlineBeginFound = False
        self.snippetTextBeginFound = False
        self.counter = 0

    def GetTagNamesToSkipTo(self):
        return [self.snippetTagName]

    def EndElementHandler(self, name):
        if name == self.snippetTagName:
            self.counter += 1
            if self.progressCallback and self.counter % 100 == 0:
                 self.progressCallback(self.taskId, 100*self.dataFile.tell() / float(os.path.getsize(self.dataFilePath)))

            self.snippetBeginFound = False
            self.currentQuery = self.currentQuery.split("url:")[0]
            if self.currentSnippetText == "":
                self.currentSnippetText = self.currentHeadline
            if (not self.collectSnippets) or (self.currentUrl.strip() != ""):
                self.parsedData.append((self.currentQuery.strip(), self.currentUrl.strip(), self.queryCharacteristics, self.queryExtraInfo, self.currentTitle.strip(), self.currentSnippetText.strip()))
            else:
                self.parsedData.append((self.currentQuery.strip(), self.currentUrl.strip(), self.queryCharacteristics, self.queryExtraInfo))
            self.currentUrl = ""
            self.currentTitle = ""
            self.currentQuery = ""
            self.queryCharacteristicsAttributeName = []
            self.currentHeadline = ""
            self.currentSnippetText = ""

        if name == self.queryTagName:
            self.queryBeginFound = False
            self.queryCharacteristicsAttributeName = []
        elif name == self.titleTagName:
            self.titleBeginFound = False
        elif name == self.headlineTagName:
            self.headlineBeginFound = False
        elif name == self.snippetTextTagName:
            self.snippetTextBeginFound = False
        elif name == self.urlTagName:
            self.urlBeginFound = False

    def CharacterDataHandler(self, data):
        #data = encode(data, "utf8")
        if self.queryBeginFound:
            self.currentQuery += data
        elif self.titleBeginFound:
            self.currentTitle += data
        elif self.urlBeginFound:
            self.currentUrl += data
        elif self.headlineBeginFound:
            self.currentHeadline += data
        elif self.snippetTextBeginFound:
            self.currentSnippetText += data

class SnippetsParser:
    ParserFileFormats = { "TabSeparated" : 1, "XmlSnippetsFormat" : 3, "XmlSerpFormat" : 2, "XmlDownloaderFormat" : 4, "XmlSnippetsNewFormat" : 5}

    def __init__(self, dataFile, dataFormat, taskId = -1, progressCallback = None):
        self.error = ""
        self.dataFile = dataFile
        self.dataFormat = dataFormat
        self.taskId = taskId
        self.progressCallback = progressCallback
        self.data = []

    def __openFile(self):
        if self.dataFile.endswith(".gz"):
            writeToLog("Open zipped file: " + self.dataFile)
            self.inputFile = gzip.open(self.dataFile, "r")
            fileObj = self.inputFile.fileobj
        else:
            writeToLog("Open file: " + self.dataFile)
            self.inputFile = open(self.dataFile, "r")
            fileObj = self.inputFile
        return fileObj

    @staticmethod
    def parseQueryExtraInfoDict(queryExtraInfoStr):
        queryExtraInfo = {}
        for qei in queryExtraInfoStr.strip().split("|"):
            qei = qei.split(":")
            if len(qei) < 2:
                continue
            queryExtraInfo[qei[0]] = qei[1].strip(";").split(";")
        return queryExtraInfo

    def _parseTabSeparatedSerpFile(self):
        serp = []
        self.__openFile()
        for line in self.inputFile:
            fields = line.strip("\r\n").split("\t")
            query, url, characteristic, extraInfoStr = fields[:4]
            characteristic = characteristic.strip("\r\n").split(";")
            extraInfo = SnippetsParser.parseQueryExtraInfoDict(extraInfoStr)
            serp.append((query, url, characteristic, extraInfo))
        self.inputFile.close()
        return serp

    def _parseTabSeparatedQueriesFile(self):
        queries = []
        self.__openFile()
        for line in self.inputFile:
            fields = line.strip("\r\n").split("\t")
            if len(fields) < 1:
                continue
            region = fields[1].strip() if len(fields) > 1 and fields[1].strip() != "" else get_option_value("defaultQueryRegion")
            url = fields[2].strip() if len(fields) > 2 else ""
            characteristic = fields[3].strip("\r\n").split(";") if len(fields) > 3 else []
            extraInfo = SnippetsParser.parseQueryExtraInfoDict(fields[4]) if len(fields) > 4 else {}
            queries.append((fields[0], region, url, characteristic, extraInfo))
        self.inputFile.close()
        return queries

    def __parsing(self, handler):
        def newParser():
            p = expat.ParserCreate()
            p.StartElementHandler = handler.StartElementHandler
            p.EndElementHandler = handler.EndElementHandler
            p.CharacterDataHandler = handler.CharacterDataHandler
            return p
        parser = newParser()
        currentFilePos = 0
        parsedToFinish = False
        writeToLog("[start] Parsing: " + self.dataFile)
        while not parsedToFinish:
            try:
                parser.ParseFile(self.inputFile)
                parsedToFinish = True
            except expat.ExpatError:
                currentFilePos += parser.ErrorByteIndex
                self.inputFile.seek(currentFilePos)
                parsedToFinish = False
                handler.Reset()
                #Now we need to skip wrong data part
                seekPos = -1024
                foundPos = -1
                while foundPos < 0:
                    seekPos += 1024
                    temp = self.inputFile.read(1024)
                    if len(temp) == 0:
                        break
                    for tag in handler.GetTagNamesToSkipTo():
                        foundPos = temp.find("<"+tag)
                        if foundPos >= 0:
                            break
                if foundPos < 0:
                    parsedToFinish = True
                else:
                    self.inputFile.seek(currentFilePos + seekPos + foundPos)
                    parser = newParser() # Recreate parser
        self.inputFile.close()
        writeToLog("[end] Parsing: " + self.dataFile)

    def _parseXmlQueryUrlFile(self):
        writeToLog("_parseXmlQueryUrlFile " + self.dataFile)
        handler = None
        fileObj = self.__openFile()
        if self.dataFormat == SnippetsParser.ParserFileFormats["XmlSerpFormat"]:
            handler = XmlSerpFormatParsingHandler(False, self.dataFile, fileObj, self.taskId, self.progressCallback)
        elif self.dataFormat == SnippetsParser.ParserFileFormats["XmlSnippetsFormat"]:
            handler = XmlSnippetsFormatParsingHandler(False, self.dataFile, fileObj, self.taskId, self.progressCallback)
        elif self.dataFormat == SnippetsParser.ParserFileFormats["XmlDownloaderFormat"]:
            handler = XmlDownloaderFormatParsingHandler(False, self.dataFile, fileObj, self.taskId, self.progressCallback)
        else:
            self.error += "/nWrong file format"
            raise Exception("Wrong file format.")
        self.__parsing(handler)
        return handler.parsedData

    def _onSnippetCallback(self, poolName, qdPair, snippet):
        self.counter += 1
        if self.progressCallback and self.counter % 100 == 0:
            self.progressCallback(self.taskId, 100*self.inputFile.tell() / float(os.path.getsize(self.dataFile)))
        self.parsedData.append((qdPair.query.strip(), qdPair.url.strip(), "", "", snippet.title.strip(), "...".join([x.text for x in snippet.stringFragments])))

    def _parseXmlDumpFile(self):
        handler = None
        fileObj = self.__openFile()
        if self.dataFormat == SnippetsParser.ParserFileFormats["XmlSerpFormat"]:
            handler = XmlSerpFormatParsingHandler(True, self.dataFile, fileObj, self.taskId, self.progressCallback)
        elif self.dataFormat == SnippetsParser.ParserFileFormats["XmlSnippetsFormat"]:
            handler = XmlSnippetsFormatParsingHandler(True, self.dataFile, fileObj, self.taskId, self.progressCallback)
        elif self.dataFormat == SnippetsParser.ParserFileFormats["XmlDownloaderFormat"]:
            handler = XmlDownloaderFormatParsingHandler(True, self.dataFile, fileObj, self.taskId, self.progressCallback)
        elif self.dataFormat == SnippetsParser.ParserFileFormats["XmlSnippetsNewFormat"]:
            self.parsedData = []
            self.counter = 0
            handler = snippet_dump_xml.SnippetDumpXmlReaderHandler(self._onSnippetCallback)
        else:
            self.error += "\nWrong file format"
            raise Exception("Wrong file format")
        self.__parsing(handler)
        return handler.parsedData if self.dataFormat != SnippetsParser.ParserFileFormats["XmlSnippetsNewFormat"] else self.parsedData

    def _parseTabSeparatedDumpFile(self):
        serp = []
        if self.dataFormat == SnippetsParser.ParserFileFormats["TabSeparated"]:
            self.__openFile()
            try:
                i = 0
                for line in self.inputFile:
                    query, url, characteristic, query_extrainfo, title, snippet  = line.strip("\r\n").split("\t")
                    characteristic = characteristic.strip("\r\n").split(";")
                    query_extrainfo_dict = SnippetsParser.parseQueryExtraInfoDict(query_extrainfo)
                    serp.append((query, url, characteristic, query_extrainfo_dict, title, snippet))
                    i += 1
                    if self.progressCallback and i % 100 == 0:
                        self.progressCallback(self.taskId, 100*self.inputFile.tell() / float(os.path.getsize(self.dataFile)))
                self.inputFile.close()
            except:
                raise
        else:
            self.error += "\nWrong file format"
            raise Exception("Wrong file format")
        return serp

    def ParseSnippets(self):
        if self.dataFormat == SnippetsParser.ParserFileFormats["TabSeparated"]:
            return self._parseTabSeparatedDumpFile()
        elif self.dataFormat == SnippetsParser.ParserFileFormats["XmlSnippetsFormat"] or self.dataFormat == SnippetsParser.ParserFileFormats["XmlSerpFormat"] or self.dataFormat == SnippetsParser.ParserFileFormats["XmlDownloaderFormat"] or self.dataFormat == SnippetsParser.ParserFileFormats["XmlSnippetsNewFormat"]:
            return self._parseXmlDumpFile()
        else:
            self.error += "/nWrong file format"
            raise Exception("Wrong file format")

    def ParseQueryUrlsFile(self):
        if self.dataFormat == SnippetsParser.ParserFileFormats["TabSeparated"]:
            return self._parseTabSeparatedSerpFile()
        elif self.dataFormat == SnippetsParser.ParserFileFormats["XmlSnippetsFormat"] or self.dataFormat == SnippetsParser.ParserFileFormats["XmlSerpFormat"] or self.dataFormat == SnippetsParser.ParserFileFormats["XmlDownloaderFormat"]:
            return self._parseXmlQueryUrlFile()
        else:
            self.error += "/nWrong file format"
            raise Exception("Wrong file format")

    def ParseQueriesFile(self):
        writeToLog("ParseQueriesFile")
        if self.dataFormat == SnippetsParser.ParserFileFormats["TabSeparated"]:
            return self._parseTabSeparatedQueriesFile()
        else:
            self.error += "/nWrong file format"
            raise Exception("Wrong file format")

if __name__ == "__main__":
    p = SnippetsParser(argv[1], SnippetsParser.ParserFileFormats["XmlSerpFormat"])
    print p.ParseSnippets()


