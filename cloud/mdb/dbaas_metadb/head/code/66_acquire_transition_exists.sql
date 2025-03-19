CREATE OR REPLACE FUNCTION code.acquire_transition_exists(
    dbaas.worker_queue
) RETURNS boolean AS $$
SELECT EXISTS (
    SELECT 1
      FROM code.cluster_status_acquire_transitions() t
      JOIN dbaas.clusters c ON (c.status = t.from_status)
     WHERE c.cid = $1.cid
       AND t.action = code.task_type_action($1.task_type)
);
$$ LANGUAGE SQL STABLE;
