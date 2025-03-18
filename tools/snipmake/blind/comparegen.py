'''
@author: ashishkin
'''

import sys
import codecs
import random 
import snippet
import tpl

if len(sys.argv) < 4:
    print "Wrong count of arguments!"
    sys.exit(1)


# options
hideEqual = False
randomize = False

options = sys.argv[1].strip()

if options != '00' and options != '10' and options != '01' and options != '11':
    print "Wrong option format!"
    sys.exit(1) 
else:
    if options.startswith("1"):
        hideEqual = True
    if options.endswith("1"):
        randomize = True

snippets1 = snippet.SnippetIO.read(sys.argv[2])
snippets2 = snippet.SnippetIO.read(sys.argv[3])

if len(snippets1) != len(snippets2):
    print "Snippet files not the same length!"
    sys.exit(1)
    
output = codecs.open(sys.argv[4], "w", "utf-8")

output.write(tpl.render(tpl.header, {"eq": hideEqual, "rnd": randomize} ))
for i in range(0, len(snippets1)):
    s1 = snippets1[i]
    s2 = snippets2[i]
    query = s1.query
    hide = False
    if hideEqual and s1.equals(s2):
        hide = True
        
    if not hide:
        output.write(tpl.render(tpl.queryRow, {"q": query}))
        
        if randomize:
            n = random.random()
            
            if n > 0.5:
                output.write(tpl.render(tpl.snipRow, {"snipTitle1": s2.title, "snipTitle2": s1.title, "snipTxt1": s2.snippet, "snipTxt2": s1.snippet, "snipUrl1": s2.url, "snipUrl2": s1.url, "left": "s2", "right": "s1", "num": i}))
            else:
                output.write(tpl.render(tpl.snipRow, {"snipTitle1": s1.title, "snipTitle2": s2.title, "snipTxt1": s1.snippet, "snipTxt2": s2.snippet, "snipUrl1": s1.url, "snipUrl2": s2.url, "left": "s1", "right": "s2", "num": i}))
        else:
            output.write(tpl.render(tpl.snipRow, {"snipTitle1": s1.title, "snipTitle2": s2.title, "snipTxt1": s1.snippet, "snipTxt2": s2.snippet, "snipUrl1": s1.url, "snipUrl2": s2.url, "left": "s1", "right": "s2", "num": i}))

output.write(tpl.render(tpl.footer, {}))
output.close()

print "Done!"    
