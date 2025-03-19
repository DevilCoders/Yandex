GRANT USAGE ON SCHEMA dbaas TO mdb_event_producer;

GRANT SELECT, INSERT, UPDATE ON TABLE dbaas.worker_queue_events TO mdb_event_producer;
GRANT SELECT ON TABLE dbaas.worker_queue TO mdb_event_producer;

