#!/usr/bin/env python
import psycopg2
import logging
import time
import argparse

logging.basicConfig(
    level=logging.DEBUG, format='%(asctime)s %(levelname)-8s: %(message)s')
LOG = logging.getLogger()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--heap', help='Check PostgreSQL heaps for corruptions', action='store_true')
    parser.add_argument('--index', help='Check PostgreSQL indexes for corruptions', action='store_true')
    parser.add_argument('--patched_index', help='Check PostgreSQL indexes for corruptions', action='store_true')
    args = parser.parse_args()
    conn = psycopg2.connect('dbname=postgres connect_timeout=1')
    cur = conn.cursor()
    cur.execute("SELECT datname FROM pg_database WHERE datistemplate = false AND datname != 'postgres'")
    dbnames = cur.fetchall()
    cur.execute("SELECT master FROM repl_mon LIMIT 1")
    master = cur.fetchone()[0]
    conn.close()
    if args.heap:
        db_checker = HeapCorruptionChecker()
    elif args.index:
        db_checker = IndexCorruptionChecker()
    elif args.patched_index:
        db_checker = PatchedIndexChecker()
    else:
        print("Use pg_corruption_check with --heap, --patched_index or --index")
        return 1

    with open(db_checker.filepath, 'w') as corr_file:
        for dbname in dbnames:
            for oid, schema, db_object, code, error in db_checker.check_db(dbname[0]):
                if code == '57014':
                    LOG.error("Check timeout: {index}".format(index=db_object))
                elif code == 'XX002':
                    LOG.error("Corrupted index: {oid} {index} {code} {error}".format(index=db_object, code=code, oid=oid, error=error))
                elif code == 'XX001':
                    LOG.error("Corrupted heap: {oid} {relation} {code} {error}".format(relation=db_object, code=code, oid=oid, error=error))
                corr_file.write('\t'.join([master, dbname[0], schema, db_object, code, error]) + '\n')


class DBCorruptionChecker(object):
    def __init__(self, filepath, select_query, check_query, extension_name, column_index):
        self.filepath = filepath
        self.select_query = select_query
        self.check_query = check_query
        self.extension_name = extension_name
        self.column_index = column_index

    def check_db(self, dbname):
        conn = psycopg2.connect('dbname={} connect_timeout=1'.format(dbname))
        conn.autocommit = True
        cur = conn.cursor()
        cur.execute("SELECT pg_is_in_recovery()")
        if cur.fetchone()[0] is False:
            cur.execute("drop extension if exists amcheck;")
            cur.execute("drop extension if exists heapcheck;")
            cur.execute("create extension if not exists amcheck;")
            cur.execute("create extension if not exists heapcheck;")
        else:
            cur.execute("SELECT * FROM pg_available_extensions WHERE name='{}' AND installed_version IS NOT NULL;".format(self.extension_name))
            fetched = cur.fetchone()
            is_installed = fetched is not None and len(fetched) > 0
            if not is_installed:
                LOG.warning("Extension {} is not installed and replica can't create it".format(self.extension_name))
                exit(1)
        cur.execute(self.select_query)
        db_objects = cur.fetchall()
        conn.close()
        for db_object in db_objects:
            conn = psycopg2.connect("dbname={} connect_timeout=1 options='-c statement_timeout=600s'".format(dbname))
            cur = conn.cursor()
            try:
                cur.execute(self.check_query.format(db_object[self.column_index]))
            except psycopg2.Error as err:
                yield (db_object[0], db_object[2], db_object[1], err.pgcode, err.pgerror.replace('\n', ' '))
            finally:
                conn.close()


class HeapCorruptionChecker(DBCorruptionChecker):
    def __init__(self):
        filepath = '/tmp/corrupted_relations'
        select_query = """
        SELECT c.oid, c.relname::text, n.nspname
        FROM pg_class c
            JOIN pg_namespace n ON c.relnamespace = n.oid
        WHERE c.relkind in('r', 't', 'm')
        """
        check_query = "select heap_check('{}')"
        extension_name = "heapcheck"
        column_index = 0
        super(HeapCorruptionChecker, self).__init__(filepath, select_query, check_query, extension_name, column_index)


class IndexCorruptionChecker(DBCorruptionChecker):
    def __init__(self):
        filepath = '/tmp/corrupted_indexes'
        select_query = """
        SELECT c.oid, c.relname, n.nspname
        FROM pg_index i
            JOIN pg_opclass op ON i.indclass[0] = op.oid
            JOIN pg_am am ON op.opcmethod = am.oid
            JOIN pg_class c ON i.indexrelid = c.oid
            JOIN pg_namespace n ON c.relnamespace = n.oid
        WHERE am.amname = 'btree'
        AND c.relpersistence != 't'
        AND i.indisready AND i.indisvalid
        ORDER BY c.relpages
        """
        check_query = "select bt_index_check({})"
        extension_name = "amcheck"
        column_index = 0
        super(IndexCorruptionChecker, self).__init__(filepath, select_query, check_query, extension_name, column_index)


class PatchedIndexChecker(IndexCorruptionChecker):
    def __init__(self):
        super(PatchedIndexChecker, self).__init__()
        self.extension_name = "heapcheck"
        self.check_query = "select patched_index_check({})"


if __name__ == '__main__':
    main()

