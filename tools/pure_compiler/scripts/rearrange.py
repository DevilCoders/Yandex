import re
import sys

if len(sys.argv) != 2:
    print >> sys.stderr, "Usage: python rearrange.py HEADER_FILE"
    sys.exit(1)

langRE = re.compile("^L(.+)\|(.+)$")
headerFile = open(sys.argv[1], "wb")
headerKey = "Language: "
headerKeyLen = len(headerKey)
for line in sys.stdin:
    line = line.rstrip()
    if line.startswith(headerKey):
        print >>headerFile, line[headerKeyLen:]
        continue
    lang = "mis"
    isForm = False
    isTitle = False
    langMatch = langRE.match(line)
    if langMatch:
       lang, line = langMatch.group(1, 2)
    if line[0] == "!":
       isForm = True
       line = line[1:]
    key, val = line.split("\t")
    if key[-1] == "\x01":
       isTitle = True
       key = key[:-1]
    print "\t".join((key, lang, str((2 if isForm else 0) + (1 if isTitle else 0)), val))
headerFile.close()
