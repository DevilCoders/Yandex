#!/usr/bin/python
import sys

from pymongo import MongoClient
from pymongo import ReplicaSetConnection


conn = MongoClient("localhost:27018")
db = conn.admin.command('serverStatus')

if(sys.argv[1] =='read'):
	print db['globalLock']['currentQueue']['readers']
elif(sys.argv[1] =='write'):
	print db['globalLock']['currentQueue']['writers']
