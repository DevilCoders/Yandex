CREATE OR REPLACE FUNCTION code.rev(
    dbaas.clusters
) RETURNS bigint AS $$
SELECT $1.actual_rev;
$$ LANGUAGE SQL IMMUTABLE;
