CREATE OR REPLACE FUNCTION code.round_cpu_quota(real)
RETURNS real AS $$
SELECT cast(round(cast($1 AS numeric), 2) AS real);
$$ LANGUAGE SQL IMMUTABLE;
