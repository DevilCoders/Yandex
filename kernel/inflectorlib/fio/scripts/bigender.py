import sys

lemma = ""
genders = set()
for l in sys.stdin:
  l = l.rstrip()
  if not l or "@" in l:
    continue
  if "[" not in l:
    if lemma and genders:
      print (lemma + "\t" + ";".join(sorted(genders)))
    lemma = l
    genders = set()
  else:
    form, gramms = l.replace(" ", "\t").split("\t")
    gramms = set(gramms.split(","))
    if "S" in gramms and "geo" in gramms:
      genders |= gramms.intersection(set(("m","f","n","mf")))


if lemma and genders:
  print (lemma + "\t" + ";".join(sorted(genders))).encode("utf-8")
