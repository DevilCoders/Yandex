#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import sqlite3
import cPickle
from argparse import ArgumentParser
import pymongo

import gencfg
from core.db import CURDB
from gaux.aux_mongo import get_mongo_collection


def main():
    sqlpath = os.path.join(CURDB.HISTDB_DIR, "histdb.sqlite3")
    sqlconn = sqlite3.connect(sqlpath)
    sqlc = sqlconn.cursor()

    mongocoll = get_mongo_collection('histdb')

    sqlc.execute("SELECT * FROM events")
    for event_id, event_type, event_name, event_object_id, event_params, event_date in sqlc.fetchall():
        event_params = cPickle.loads(str(event_params))
        mongocoll.insert({
            'event_id': event_id,
            'event_type': event_type,
            'event_name': event_name,
            'event_object_id': event_object_id,
            'event_params': event_params,
            'event_date': event_date,
        })


if __name__ == '__main__':
    main()
