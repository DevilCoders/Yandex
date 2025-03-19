import sys
import re

for line in sys.stdin:
    print re.sub('\n', ' ', line.decode('string_escape'))
