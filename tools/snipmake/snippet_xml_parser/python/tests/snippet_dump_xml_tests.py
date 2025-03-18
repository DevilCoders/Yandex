import random
import os
import sys
import unittest
sys.path.append('../')

from snippet_dump_xml import SnippetDumpXmlReader
from snippet_dump_xml import SnippetDumpXmlWriter
from snippet_dump_xml import SAXSnippetDumpXmlWriter

class TestSnippetDumXmlFunctions(unittest.TestCase):
    def setUp(self):
        self.testDumpXmlParser = SnippetDumpXmlReader("test_read.xml", self._processTestDumpXmlParser)
        self.testReadQDPairData = []
        self.testReadSnippetData = []
    
    def _processTestDumpXmlParser(self, pool, qdPairData, snippetData):
        self.testReadQDPairData.append(qdPairData)
        self.testReadSnippetData.append(snippetData)

    # ----------------------------- Read Test ------------------------------------------
    def read_xml_check(self):
        self.assertEqual(len(self.testReadQDPairData), 1)
        self.assertEqual(len(self.testReadSnippetData), 1)
        
        # qdPairData
        testQDPairData = self.testReadQDPairData[0]
        self.assertEqual(testQDPairData.query, "request")
        self.assertEqual(testQDPairData.region, "213")
        self.assertEqual(testQDPairData.richtree, "2B3D3D")
        self.assertEqual(testQDPairData.url, "url.html")
        self.assertEqual(testQDPairData.relevance, "20")
        
        # snippetData
        testSnippetData = self.testReadSnippetData[0]
        self.assertEqual(len(testSnippetData.fragments), 2)
        self.assertEqual(len(testSnippetData.marks), 2)
        self.assertTrue(not testSnippetData.comment == None)
        
        self.assertEqual(testSnippetData.algorithm, "Algo1")
        self.assertEqual(testSnippetData.rank, "333")
        self.assertEqual(testSnippetData.title, "Title1")
        self.assertEqual(testSnippetData.featuresString, "features")
        
        self.assertEqual(testSnippetData.fragments[0].coords, "0-111")
        self.assertEqual(testSnippetData.fragments[0].text, "Fragment1")
        self.assertEqual(testSnippetData.fragments[1].coords, "222-333")
        self.assertEqual(testSnippetData.fragments[1].text, "Fragment2")

        self.assertEqual(testSnippetData.featuresString, "features")
        
        self.assertEqual(testSnippetData.marks[0].value, "3")
        self.assertEqual(testSnippetData.marks[0].criteria, "readability")
        self.assertEqual(testSnippetData.marks[0].assessor, "A-Babina")
        self.assertEqual(testSnippetData.marks[0].quality, "0.86")
        self.assertEqual(testSnippetData.marks[0].timestamp, "1270115080")
        
        self.assertEqual(testSnippetData.marks[1].value, "-100")
        self.assertEqual(testSnippetData.marks[1].criteria, "content")
        self.assertEqual(testSnippetData.marks[1].assessor, "Kolyan")
        self.assertEqual(testSnippetData.marks[1].quality, "0.1")
        self.assertEqual(testSnippetData.marks[1].timestamp, "1170115180")
        
        self.assertEqual(testSnippetData.comment.assessor, "usminski")
        self.assertEqual(testSnippetData.comment.tag, "classic_fail_3 classic_fail_2")
        self.assertEqual(testSnippetData.comment.text, "This is the most stupid snippet I've ever seen!")
        
    def test_read_xml(self):
        self.testDumpXmlParser.StartParse()
        self.read_xml_check()
        
    # ----------------------------- Write Test ------------------------------------------
    def test_write_xml(self):
        self.testDumpXmlParser.StartParse()
    
        writer = SAXSnippetDumpXmlWriter("test_out.xml")
        writer.StartWrite()
        writer.AppendQDPair(self.testReadQDPairData[0], self.testReadSnippetData)        
        writer.EndWrite()
        
        self.testReadQDPairData = []
        self.testReadSnippetData = []
        
        testDumpXmlParser = SnippetDumpXmlReader("test_out.xml", self._processTestDumpXmlParser)
        testDumpXmlParser.StartParse()        
        self.read_xml_check()
    
if __name__ == '__main__':
    unittest.main()
