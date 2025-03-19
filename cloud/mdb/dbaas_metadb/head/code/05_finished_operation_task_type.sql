CREATE OR REPLACE FUNCTION code.finished_operation_task_type() RETURNS text AS $$
SELECT 'finished_before_starting'::text;
$$ LANGUAGE SQL IMMUTABLE;
