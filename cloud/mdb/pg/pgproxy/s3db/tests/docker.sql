DROP ROLE IF EXISTS monitor;
CREATE ROLE monitor;

\i ../pgproxy.sql
\i ../inc_cluster_version.sql

CREATE OR REPLACE FUNCTION plproxy.dynamic_query_meta(q text)
RETURNS SETOF RECORD AS $$
    CLUSTER 'meta_rw';
    RUN ON ALL;
    TARGET dynamic_query;
$$ LANGUAGE plproxy;

CREATE OR REPLACE FUNCTION plproxy.dynamic_query_db(q text)
RETURNS SETOF RECORD AS $$
    CLUSTER 'db_rw';
    RUN ON ALL;
    TARGET dynamic_query;
$$ LANGUAGE plproxy;

DROP ROLE IF EXISTS s3api;
CREATE ROLE s3api;

DROP ROLE IF EXISTS s3api_ro;
CREATE ROLE s3api_ro;

DROP ROLE IF EXISTS s3api_list;
CREATE ROLE s3api_list;

DROP ROLE IF EXISTS s3cleanup;
CREATE ROLE s3cleanup;

\i plproxy.sql
\i update_remote_tables.sql
