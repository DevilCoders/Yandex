import libxml2
import libxslt
import sys

def Transform(xmlSrs, xsltSrc):
    stylesheetArgs = {} # optional transform args
    styleDoc = libxml2.parseDoc(xsltSrc) # <xml ...xsl:stylesheet >
    style = libxslt.parseStylesheetDoc(styleDoc)

    doc = libxml2.parseDoc(xmlSrs) # <xml input file>
    result = style.applyStylesheet(doc,stylesheetArgs)
    res = style.saveResultToString(result)

    style.freeStylesheet()
    doc.freeDoc()
    result.freeDoc()
    return res


if __name__ == "__main__":
    fxml = open(sys.argv[1])
    fxslt = open(sys.argv[2])
    print Transform(fxml.read(), fxslt.read()) 

