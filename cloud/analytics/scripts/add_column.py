#!/usr/bin/env python
import sys

import yt.wrapper as yt

CLUSTER = 'hume'
PREFIX = "//projects/marketo-testing/import/"

yt.config['proxy']['url'] = CLUSTER

table = PREFIX+sys.argv[1]
column = sys.argv[2]
type = sys.argv[3]

schema = yt.get(table+'/@schema')
schema.append({"name": column, "type": type})
yt.alter_table(table, schema)
