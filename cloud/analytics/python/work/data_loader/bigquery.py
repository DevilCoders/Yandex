#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import psycopg2 as pg, pandas as pd, datetime, os, pandas_gbq as gbq, numpy as np, hashlib, json, hashlib, requests, time
from google.cloud import bigquery
from requests import ConnectionError
from google.auth.exceptions import TransportError

def create_table_as_select(bq_client, dataset_name, table_name, sqlQuery, project=None):
    while True:
        try:
            job_config = bigquery.QueryJobConfig()

            # Set configuration.query.destinationTable
            dataset_ref = bq_client.dataset(dataset_name)
            table_ref = dataset_ref.table(table_name)

            job_config.destination = table_ref

            # Set configuration.query.createDisposition
            job_config.create_disposition = 'CREATE_IF_NEEDED'

            # Set configuration.query.writeDisposition
            job_config.write_disposition = 'WRITE_TRUNCATE'

            # Start the query
            job = bq_client.query(sqlQuery, job_config=job_config)

            # Wait for the query to finish
            job.result()

            returnMsg = 'Created table {}.'.format(table_name)
            break
        except (ConnectionError, TransportError):
            print('ConnectionError!\nNew try!')
            continue


def delete_rows_from_bq(query, bq_client):
    while True:
        try:
            query_job = bq_client.query(query)
            query_job.result()
            break
        except (ConnectionError, TransportError):
            print('ConnectionError!\nNew try!')
            continue

def delete_bq_table(bq_client, schema, table_name):
    while True:
        try:
            bq_client.delete_table(bq_client.dataset(schema).table(table_name))
        except (ConnectionError, TransportError):
            print('Error in delete_bq_table function')
            continue
    print('%s.%s was deleted!!!\n\n' % (schema,table_name))
