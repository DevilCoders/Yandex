CREATE OR REPLACE FUNCTION plproxy.dynamic_query_all(q text)
RETURNS SETOF RECORD AS $$
    CLUSTER 'ro';
    RUN ON ALL;
    TARGET dynamic_query;
$$ LANGUAGE plproxy;

CREATE OR REPLACE FUNCTION plproxy.dynamic_query_any(q text)
RETURNS SETOF RECORD AS $$
    CLUSTER 'ro';
    RUN ON ANY;
    TARGET dynamic_query;
$$ LANGUAGE plproxy;
