#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import psycopg2 as pg, pandas as pd

def make_connection(dbname, host, user, password):
    return pg.connect("dbname='%s' user='%s' host='%s' password='%s'" % (dbname, user, host, password))

def execute_query(query, connection, method = 'full'):
    cur = connection.cursor()
    cur.execute(query)
    if method == 'full':
        return pd.DataFrame(cur.fetchall())
    else:
        return cur

def get_data_from_db(query, connection_data):
    connection = make_connection(**connection_data)

    df = pd.read_sql_query(query, con=connection)
    connection.close()
    return df
