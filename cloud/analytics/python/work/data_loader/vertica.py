#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, vertica_python as vp


def vertica_select(query, creds):
    '''
    returns pandas DataFrame from sql-query
    '''
    vertica_connection = vp.connect(**creds)
    result = pd.read_sql(query, vertica_connection)
    vertica_connection.close()
    return result
