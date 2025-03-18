from clickhouse.client import connect


def main():
    conn = connect(host='mtstat01-1.yandex.ru', port=8123, username='john_doe', password='qwerty')
    cur = conn.cursor()
    db_name = 'dummy_example_db'
    table_name = '%s.dummy_table' % db_name
    cur.execute('CREATE DATABASE %s' % db_name)
    try:
        cur.execute('CREATE TABLE %s (RegionID UInt32, Name String) ENGINE = Memory' % table_name)
        cur.execute(
            'INSERT INTO %s' % table_name,
            data = [
                [1, 'aaa'],
                [2, 'bbb'],
                [5, 'ccc'],
                [14, 'ddd'],
                [20, 'eee']
            ]
        )
        cur.execute('SELECT * FROM %s' % table_name)
        data = cur.fetchall()
        print data
        cur.execute('DROP TABLE %s' % table_name)
    except Exception:
        raise
    finally:
        cur.execute('DROP DATABASE %s' % db_name)


if __name__ == '__main__':
    main()
