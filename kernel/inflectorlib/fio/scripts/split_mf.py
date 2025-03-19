import sys

def print_lines(lines, duplicate):
  print "\n".join(lines).replace(",mf", ",m").replace("mf,", "m,").encode("utf-8")
  print ""
  if duplicate:
    print "\n".join(lines).replace(",mf", ",f").replace("mf,", "f,").encode("utf-8")
    print ""

lines = []
duplicate = False
for l in sys.stdin:
  l = l.rstrip().decode("utf-8")
  if not l:
    continue
  elif "[" not in l:
    if lines:
      print_lines(lines, duplicate)
    lemma = l
    lines = [lemma]
    duplicate = False
  else:
    form, gram = l.replace("\t", " ").split(" ")
    grams = gram.split(",")
    if "mf" in grams:
      duplicate = True
    lines.append(l)

if lines:
  print_lines(lines, duplicate)
