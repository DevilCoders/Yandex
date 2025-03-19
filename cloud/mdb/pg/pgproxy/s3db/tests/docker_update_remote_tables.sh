#!/bin/bash
cat update_remote_tables.sql | sed -e '/--start of the section for FDW/ a\
    create server remote foreign data wrapper postgres_fdw options (host '\''pgmeta'\'', dbname '\''s3db'\'');\
    create user mapping for postgres server remote options (user '\''postgres'\'');' \
    -e '/--start of the section for grants/ a\
    grant all on all tables in schema plproxy to s3api;\
    grant execute on all functions in schema plproxy to s3api;'
