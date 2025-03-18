# -*- coding: utf-8 -*-


import codecs
import xml.sax
import xml.sax.saxutils
 

class SnippetIO(xml.sax.ContentHandler):
    #def error(self, exception):
        ##print >>sys.stderr, exception
        #raise exception

    #def fatalError(self, exception):
        ##print >>sys.stderr, exception
        #raise exception

    def __init__(self, collector):
        self.__t = "" #title
        self.__u = "" #url
        self.__d = "" #text data
        self.__q = "" #query
        self.__s = "" #search engine name
        self.__h = "" #headline
        self.__c = collector
        
        #internal flags needed when parse
        self.__startsnip = False
        self.__buffer = u""
        
    def startElement(self, name, attrs):
        if name == "snippet":
            self.__startsnip = True
            for k, v in attrs.items():
                if k == "seName":
                    self.__s = xml.sax.saxutils.unescape(v)
        
    def characters(self, ch):
        if self.__startsnip == True:
            self.__buffer = self.__buffer + ch
                
    def endElement(self, name):
        if self.__startsnip == True:
            if name == "query":
                self.__q = self.__buffer
            elif name == "url":
                self.__u = self.__buffer
            elif name == "title":
                self.__t = self.__buffer
            elif name == "text":
                self.__d = self.__buffer
            elif name == "headline":
                self.__h = self.__buffer
        if name == "snippet":
            self.__c.append(Snippet(self.__u, self.__t, self.__h, self.__d, self.__q, self.__s))
            self.__startsnip = False
        self.__buffer = u""            
            
    @staticmethod
    def read(file):
        f = codecs.open(file, "r", "utf-8")
        xmlString = unicode(f.read())
        f.close()
        readString(xmlString)

    @staticmethod
    def readString(xmlString):
        snippets = []
        xml.sax.parseString(xmlString, SnippetIO(snippets))
        return snippets
    
    @staticmethod    
    def write(snippets, filename):
        output = codecs.open(filename, "w", "utf-8")
        
        output.write("<?xml version=\"1.0\" ?>\n\n")        
        output.write("<queries>\n")
        
        for s in snippets:
             output.write("<snippet seName=\"" + Snippet.__cleanup__(s.seName).strip() + "\">\n")
             output.write("\t<query><![CDATA[" + Snippet.__cleanup__(s.query).strip() + "]]></query>\n")
             output.write("\t<url><![CDATA[" +  Snippet.__cleanup__(s.url).strip() + "]]></url>\n")
             output.write("\t<title><![CDATA[" +  Snippet.__cleanup__(s.title).strip() + "]]></title>\n")
             output.write("\t<headline><![CDATA[" + Snippet.__cleanup__(s.headline).strip()  + "]]></headline>\n")
             output.write("\t<text><![CDATA[" +   Snippet.__cleanup__(s.snippet).strip() + "]]></text>\n")
             output.write("</snippet>\n")
             
        output.write("</queries>")
        output.close()
        
class Snippet:
    """ Defines simple class to store parsed snippets.
    
        @author: ashishkin
    """
        
    def __init__(self, url, title = "", headline = "", snip = "", query = "", seName = ""):
        """ Constructor. 
            Creates instance inited by snippet title, snippet text and search query
        
            @param url: document url
            @param title: snippet title
            @param snip: snippet text 
            @param query: search query for snippet 
            @param encoding: encoding of snippets 
        """
        self.url = url
        self.title = title
        self.headline = headline
        self.snippet = snip
        self.query = query
        self.seName = seName
        
    def getSnippet(self):
        return self.title + u" " + u" ".join(self.snippet)
    
    def equals(self, other):
        if isinstance(other, Snippet):
            return other.query == self.query and other.url == self.url and other.title == self.title and other.headline == self.headline and other.snippet == self.snippet and other.seName == self.seName 
        return False
        
    @staticmethod    
    def __cleanup__(str):
        from StringIO import StringIO
        res = StringIO()
        chars = set("\"',.-!?&/\\:;#@%*()[]<>~+=|`~№$^_ ")
        for i in range(0, len(str)):
            if str[i].isalnum() or str[i] in chars:
                res.write(str[i])
        string = res.getvalue()
        res.close()
        return string
        
        
if __name__ == "__main__":
    import sys
    snippets = SnippetIO.read(sys.argv[1])
    print len(snippets)
    
