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
