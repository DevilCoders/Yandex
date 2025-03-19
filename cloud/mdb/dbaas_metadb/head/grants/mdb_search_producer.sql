GRANT USAGE ON SCHEMA dbaas TO mdb_search_producer;

GRANT SELECT, INSERT, UPDATE ON TABLE dbaas.search_queue TO mdb_search_producer;
GRANT ALL ON SEQUENCE dbaas.search_queue_queue_ids TO mdb_search_producer;
