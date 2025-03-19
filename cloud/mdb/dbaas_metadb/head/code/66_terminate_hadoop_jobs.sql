CREATE OR REPLACE FUNCTION code.terminate_hadoop_jobs(
   i_cid                   text
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.hadoop_jobs
       SET status = 'ERROR',
           start_ts = COALESCE(start_ts, now()),
           end_ts = COALESCE(end_ts, now()),
           comment = 'The job was terminated by cluster operation.'
     WHERE cid = i_cid
       AND status NOT IN ('ERROR', 'DONE', 'CANCELLED');

    UPDATE dbaas.worker_queue
       SET start_ts = COALESCE(start_ts, now()),
           end_ts = COALESCE(end_ts, now()),
           errors = '[{"code": 1, "type": "Cancelled", "message": "The job was terminated", "exposable": true}]'::jsonb,
           result = false
     WHERE cid = i_cid 
       AND result is null
       AND unmanaged = true;

END;
$$ LANGUAGE plpgsql;
