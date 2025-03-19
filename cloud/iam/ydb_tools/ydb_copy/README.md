### YDB table copy utility


```bash
ydb_copy src_table dst_table
    --batch-size 100
    --column column_to_copy_1
    --column column_to_copy_2
    --column column_to_copy_n
    --timeout 1800
    --verbose
    --only-upsert
    --src-endpoint localhost:2135
    --src-database old_db
    --src-auth-token token.txt
    --dst-endpoint localhost:2135
    --dst-database new_db
    --dst-auth-token token.txt
```

If no columns are specified, all columns will be copied.
The PK columns should be included.

### Examples

##### Copy table XXX from DB1 to DB2

```bash
ydb_copy /DB1/XXX /DB2/XXX \
  --src-endpoint some-ydb-host:2135 --src-database DB1 \
  --dst-endpoint some-ydb-host:2135 --dst-database DB2
```

##### Copy columns XXX, YYY, ZZZ from src_table to dst_table

```bash
ydb_copy src_table dst_table --column XXX --column YYY --column ZZZ
```

##### Set column XXX to literal 'STR' using column YYY as PK

```bash
ydb_copy src_table dst_table --column YYY --column "'STR' as XXX"
```

##### Copy column XXX from src_table to dst_table as YYY

```bash
ydb_copy src_table dst_table --column "XXX as YYY"
```

