import sys

for line in sys.stdin:
    key, lang, freq = line.rstrip().split("\t")
    isForm = False
    isTitle = False
    if key[0] == "!":
       isForm = True
       key = key[1:]
    if key[-1] == "\x01":
       isTitle = True
       key = key[:-1]
    print "\t".join((key, lang, str((2 if isForm else 0) + (1 if isTitle else 0)), freq))
