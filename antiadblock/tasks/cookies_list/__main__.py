from __future__ import print_function
import psycopg2
import os
import json

conn_str = os.getenv("PG_CONN_STR")

conn = psycopg2.connect(conn_str)
cursor = conn.cursor()
try:
    cursor.execute(
        """
    select c.data->>'WHITELIST_COOKIES' as cookies
    from configs as c join config_statuses as cs on c.id=cs.config_id
     where c.data::jsonb ? 'WHITELIST_COOKIES' and cs.status = 'active'
    """)
    cookies = set()
    rows = cursor.fetchall()
    for row in rows:
        if row[0]:
            for cookie in json.loads(row[0]):
                cookies.add(cookie)
    if len(cookies) == 0:
        raise Exception('Empty cookies')
    print('\n'.join(list(cookies)), end='')
finally:
    cursor.close()
    conn.close()
