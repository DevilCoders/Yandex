# import sys

deleted = open('groups.txt').read().replace(" None", "").replace("    ", "").split("\n")

# print deleted
# sys.exit(1)

with open('mapping.py') as f:
    for line in f:
        ls = line.strip()
        found = False
        for d in deleted:
            if not d:
                continue
            if '"{}"'.format(d) in ls:
                found = True
                break
        if not found:
            print line.rstrip()
