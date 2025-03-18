import os
import hashlib
import sys

from optparse import OptionParser
from sys import argv

sys.path.append("../snippet_xml_parser/python")
from snippet_dump_xml import SnippetDumpXmlReader
from snippet_dump_xml import SAXSnippetDumpXmlWriter
from snippet_dump_xml import QDPairData
from snippet_dump_xml import SnippetData
from snippet_dump_xml import FragmentData
from snippet_dump_xml import MarkData

QDPairDict = {}
FirstSnippetDatas = {}
SecondSnippetDatas = {}

def GetSnippetHash(query, url, region):
    return hashlib.sha224("_".join([query, url.strip("/"), region]).encode("utf-8")).hexdigest()

def GetSnippetText(snippetData):
    text = ""
    for fragment in snippetData.stringFragments:
        if len(text) > 0:
            text += "..."
        text += fragment.text
    return text

def FirstFileCallback(poolID, qdPairData, snippetData):
    qdPairHash = GetSnippetHash(qdPairData.query, qdPairData.url, qdPairData.region)
    QDPairDict[qdPairHash] = qdPairData
    FirstSnippetDatas[qdPairHash] = snippetData

def SecondFileCallback(poolID, qdPairData, snippetData):
    qdPairHash = GetSnippetHash(qdPairData.query, qdPairData.url, qdPairData.region)
    SecondSnippetDatas[qdPairHash] = snippetData

def AreEqualSnippets(snippet1, snippet2):
    return snippet1.title == snippet2.title and GetSnippetText(snippet1) == GetSnippetText(snippet2)

def FindSnippetDif(first_file_name, second_file_name, ouput_file_name, need_xml_dif):
    first_reader = SnippetDumpXmlReader(first_file_name, FirstFileCallback)
    second_reader = SnippetDumpXmlReader(second_file_name, SecondFileCallback)

    first_reader.StartParse()
    second_reader.StartParse()

    difCount = 0
    allCount = 0
    
    if need_xml_dif:
        firstXmlWriter = SAXSnippetDumpXmlWriter(ouput_file_name + "_first.xml")
        firstXmlWriter.StartWrite()
        
        secondXmlWriter = SAXSnippetDumpXmlWriter(ouput_file_name + "_second.xml")
        secondXmlWriter.StartWrite()

    with open(ouput_file_name, "w") as output_file:
        for qdPairHash in FirstSnippetDatas.keys():
            if qdPairHash in SecondSnippetDatas:
                allCount += 1
            if qdPairHash in SecondSnippetDatas and \
               not AreEqualSnippets(FirstSnippetDatas[qdPairHash], SecondSnippetDatas[qdPairHash]):

                output_file.write("#### " + QDPairDict[qdPairHash].query.encode("utf-8") + "\t" + QDPairDict[qdPairHash].url.encode("utf-8") + " #######\n")

                output_file.write(FirstSnippetDatas[qdPairHash].title.encode("utf-8") + "\n")
                output_file.write(GetSnippetText(FirstSnippetDatas[qdPairHash]).encode("utf-8") + "\n\n")

                output_file.write(SecondSnippetDatas[qdPairHash].title.encode("utf-8") + "\n")
                output_file.write(GetSnippetText(SecondSnippetDatas[qdPairHash]).encode("utf-8") + "\n")
                output_file.write("***********************************************\n\n")
                difCount += 1
                
                if need_xml_dif:
                    firstXmlWriter.AppendQDPairSnippets(QDPairDict[qdPairHash], [ FirstSnippetDatas[qdPairHash] ])
                    secondXmlWriter.AppendQDPairSnippets(QDPairDict[qdPairHash], [ SecondSnippetDatas[qdPairHash] ])

    print "Dif count: {0} ({1:.2%})".format(difCount, float(difCount) / float(allCount))
    print "All count: {0}".format(allCount)
    
    if need_xml_dif:
        firstXmlWriter.EndWrite()
        secondXmlWriter.EndWrite()

if __name__=="__main__":
    usage = '''usage: %prog <old> <new>'''
    parser = OptionParser(usage)

    parser.add_option("-f", "--first", action="store", dest="first", metavar="PATH",
                      help="first xml format file name")
    parser.add_option("-s", "--second", action="store", dest="second", metavar="PATH",
                      help="second xml format file name")
    parser.add_option("-o", "--ouput", action="store", dest="ouput", metavar="PATH",
                      help="output plain file name")
    parser.add_option("-x", "--xml", action="store_true", default = False, 
                      help="store difference into separate xml files")

    (options, args) = parser.parse_args()
    FindSnippetDif(options.first, options.second, options.ouput, options.xml)
