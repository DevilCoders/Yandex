#!/usr/bin/python

from pymongo import MongoClient
from pymongo import ReplicaSetConnection

conn = MongoClient("localhost:27018",)
db = conn.admin.command('replSetGetStatus')
if db['myState'] == 2:
	print "green"
elif db['myState'] == 1:
	print "blue"
elif db['myState'] == 7:
	print "yellow"

