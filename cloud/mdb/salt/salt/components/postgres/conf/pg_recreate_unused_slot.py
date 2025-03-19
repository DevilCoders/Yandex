#!/usr/bin/env python

import psycopg2

def main():
    conn = psycopg2.connect('dbname=postgres connect_timeout=1 options=\'-c log_statement=none\'')
    cur = conn.cursor()
    #Logical replication on replica doesn't work
    cur.execute('SELECT pg_is_in_recovery()')
    if cur.fetchone()[0]:
        cur.execute("""SELECT pg_drop_replication_slot(slot_name) FROM pg_replication_slots
                        WHERE slot_type='logical';""")

    cur.execute("""SELECT slot_name FROM pg_replication_slots
                    WHERE active = false
                    AND (xmin IS NOT NULL
                    OR restart_lsn IS NOT NULL)
                    AND slot_type='physical';""")

    for slot_name in cur.fetchall():
        cur.execute("SELECT pg_drop_replication_slot(%(slot_name)s)"
                                                      ,{'slot_name': slot_name})
        cur.execute("SELECT pg_create_physical_replication_slot(%(slot_name)s)"
                                                      ,{'slot_name': slot_name})
    cur.close()
    conn.close()

if __name__ == '__main__':
    main()
