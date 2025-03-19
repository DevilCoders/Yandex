{%- set exclude_schemas = [ 'pg_catalog' ] -%}
SELECT table_schema, table_name  from
            (SELECT table_schema,
                    table_name,
                    COALESCE (statime, create_time, '1000-01-01 00:00:00') AS statime
             FROM (SELECT table_schema, table_name, statime, create_time
                   FROM (SELECT n.nspname AS table_schema,
                                c.relname AS table_name,
                                o.statime AS statime,
								NULL AS create_time
                         FROM pg_class c
                         LEFT JOIN pg_namespace n ON c.relnamespace = n.oid
                         LEFT JOIN pg_stat_operations o
                           ON o.objname = c.relname
                           AND o.schemaname = n.nspname
                         LEFT JOIN pg_partitions p
                           ON p.partitiontablename=c.relname
                           AND p.partitionschemaname=n.nspname
                   WHERE c.relkind = 'r'
                     AND c.relstorage <> 'x'
					 AND c.relpersistence != 't'
                     AND n.nspname NOT IN ({% for schema in exclude_schemas %}
                       '{{ schema }}'{% if not loop.last %},{% endif %}
                       {% endfor %}
                       )
                     AND o.actionname = 'VACUUM'
                     AND p.tablename is NULL) sq1
             UNION
             ( SELECT n.nspname AS table_schema,
                      c.relname AS table_name,
                      NULL AS statime,
					  b.statime AS create_time
               FROM pg_class c
               LEFT JOIN pg_namespace n ON c.relnamespace = n.oid
               LEFT JOIN pg_stat_operations o ON o.objname = c.relname AND o.schemaname = n.nspname
               LEFT JOIN (SELECT *
                          FROM pg_stat_operations
                          WHERE actionname = 'VACUUM') a
                          ON a.objname = c.relname AND a.schemaname = n.nspname
               LEFT JOIN pg_partitions p
                 ON p.partitiontablename=c.relname AND p.partitionschemaname=n.nspname
  			   LEFT JOIN (SELECT *
                            FROM pg_stat_operations
                            WHERE actionname = 'CREATE') b
                            ON b.objname = c.relname AND b.schemaname = n.nspname			
               WHERE c.relkind = 'r'
                 AND c.relstorage <> 'x'
				 AND c.relpersistence != 't'
                 AND n.nspname NOT IN ({% for schema in exclude_schemas %}
                   '{{ schema }}'{% if not loop.last %},{% endif %}
                   {% endfor %}
                   )
                 AND a.actionname IS NULL
                 AND p.tablename is NULL
               GROUP BY n.nspname, c.relname, b.statime)) sq2
               ORDER BY statime) sq3
