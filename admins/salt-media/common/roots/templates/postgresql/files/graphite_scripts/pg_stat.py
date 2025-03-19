#!/usr/bin/env python

import time
import psycopg2
import socket
import re
import os

db_user = "monitor"
db_pass = None

dbs_except = ["template0", "template1", "postgres"]

db_stat_fields = ["datname", "xact_commit", "xact_rollback",
                  "blks_read", "blks_hit", "tup_returned",
                  "tup_fetched", "tup_inserted", "tup_updated",
                  "tup_deleted", "conflicts", "deadlocks",
                  "blk_read_time", "blk_write_time"]

pg_sut_fields = ["schemaname", "relname", "seq_scan",
                 "seq_tup_read", "idx_scan", "idx_tup_fetch",
                 "n_tup_ins", "n_tup_upd", "n_tup_del",
                 "n_tup_hot_upd", "n_live_tup", "n_dead_tup",
                 "autovacuum_count", "autoanalyze_count"]

pg_siout_fields = ["schemaname", "relname", "heap_blks_read",
                   "heap_blks_hit", "idx_blks_read", "idx_blks_hit",
                   "toast_blks_read", "toast_blks_hit",
                   "tidx_blks_read", "tidx_blks_hit"]

pg_sui_fields = ["indexrelname", "idx_scan",
                 "idx_tup_read", "idx_tup_fetch"]

pg_sioui_fields = ["indexrelname", "idx_blks_read", "idx_blks_hit"]


def None20(val):
    if val is None:
        return("0")
    else:
        return(val)

try:
    #conn = psycopg2.connect("host=localhost port=5432 dbname=postgres " +
    conn = psycopg2.connect("dbname=postgres " +
                            "user=%s password=%s " % (db_user, db_pass) +
                            "connect_timeout=1")
    cur = conn.cursor()

except psycopg2.OperationalError:
    print("Error occurred while connecting to a database")
    exit(1)

cur.execute("select %s from pg_stat_database" % (", ". join(db_stat_fields)))

for db_record in cur.fetchall():
    db_datname = db_record[0]

    if db_datname in dbs_except:
        continue

    try:
        gr_db_datname = re.findall("^([\d\w_]+)_p\d+?$", db_datname)[0]

    except IndexError:
        gr_db_datname = db_datname

    for dbf in range(1, len(db_stat_fields)):
        print "db.%s.%s %s" % (gr_db_datname, db_stat_fields[dbf],
                                        None20(db_record[dbf]))

    try:
        conn_db = psycopg2.connect("dbname=%s " % db_datname +
                                   "user=%s " % db_user +
                                   "password=%s " % db_pass +
                                   "connect_timeout=1")
        cur_db = conn_db.cursor()

    except:
        print "Error occured while connecting to a database %s" % db_datname
        continue

    cur_db.execute("select %s " % (", ". join(pg_sut_fields)) +
                   "from pg_stat_user_tables")

    for tbl_record in cur_db.fetchall():
        tbl_relname = tbl_record[0].replace('.', '_') + '_' + \
            tbl_record[1].replace('.', '_')

        for tblf in range(2, len(pg_sut_fields)):
            print "db.%s.table.%s.%s %s" % (gr_db_datname,
                                                     tbl_relname,
                                                     pg_sut_fields[tblf],
                                                     None20(tbl_record[tblf]))

        cur_tmp = conn_db.cursor()

        tpl = "%s.%s" % (tbl_record[0], tbl_record[1])
        cur_tmp.execute("select size, size_total from " +
                        "pg_relation_size('%s') as size, " % tpl +
                        "pg_total_relation_size('%s') as size_total" % tpl)
        rel_size, rel_total_size = cur_tmp.fetchone()
        print "db.%s.table.%s.%s %s" % (gr_db_datname,
                                                 tbl_relname, "size",
                                                 None20(rel_size))
        print "db.%s.table.%s.%s %s" % (gr_db_datname,
                                                 tbl_relname, "size_total",
                                                 None20(rel_total_size))

    cur_db.execute("select %s " % (", ".join(pg_siout_fields)) +
                   "from pg_statio_user_tables")

    for tbl_record in cur_db.fetchall():
        tbl_relname = tbl_record[0].replace('.', '_') + \
            '_' + tbl_record[1].replace('.', '_')

        for tblf in range(2, len(pg_siout_fields)):
            print "db.%s.table.%s.%s %s" % (gr_db_datname,
                                                     tbl_relname,
                                                     pg_siout_fields[tblf],
                                                     None20(tbl_record[tblf]))

    cur_db.execute("select %s " % (", ".join(pg_sui_fields)) +
                   "from pg_stat_user_indexes")

    for idx_record in cur_db.fetchall():
        idx_relname = idx_record[0].replace('.', '_')

        for idxf in range(1, len(pg_sui_fields)):
            print "db.%s.index.%s.%s %s" % (gr_db_datname,
                                                     idx_relname,
                                                     pg_sui_fields[idxf],
                                                     None20(idx_record[idxf]))

    cur_db.execute("select %s " % (", ". join(pg_sioui_fields)) +
                   "from pg_statio_user_indexes")

    for idx_record in cur_db.fetchall():
        idx_relname = idx_record[0].replace('.', '_')

        for idxf in range(1, len(pg_sioui_fields)):
            print "db.%s.index.%s.%s %s" % (gr_db_datname,
                                                     idx_relname,
                                                     pg_sioui_fields[idxf],
                                                     None20(idx_record[idxf]))

    cur_db.close()
    conn_db.close()

cur.close()
conn.close()

