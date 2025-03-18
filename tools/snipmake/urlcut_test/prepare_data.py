#!/usr/bin/env python

from sys import stdin
from re import compile

QUERY_MARKER = compile(r"^---------- QUERY:\s*(.+)\s*$");
URL_MARKER = compile(r'^\s*Url:\s*"([^"]+)"\s*$') 

query = "" 

for l in stdin:
    m = QUERY_MARKER.match(l)
    
    if m:
        query = m.group(1)
        continue
    
    m = URL_MARKER.match(l)
    
    if m and query:
        print "%s\t%s" % (m.group(1), query)
