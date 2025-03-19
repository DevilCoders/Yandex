DROP ROLE IF EXISTS monitor;
CREATE ROLE monitor;
DROP ROLE IF EXISTS pgproxy;
CREATE ROLE pgproxy;
DROP ROLE IF EXISTS s3ro;
CREATE ROLE s3ro;

\i ../../pgmeta/s3db.sql

INSERT INTO clusters (cluster_id, name)
    VALUES (1, 'meta'), (2, 'db');

INSERT INTO hosts (host_id, host_name, dc, base_prio) VALUES
    (1, 's3meta01', 'LOCAL', 0),
    (2, 's3db01', 'LOCAL', 0),
    (3, 's3db02', 'LOCAL', 0);

INSERT INTO connections (conn_id, conn_string) VALUES
    (1, 'host=s3meta01 dbname=s3meta user=postgres'),
    (2, 'host=s3db01 dbname=s3db user=postgres'),
    (3, 'host=s3db02 dbname=s3db user=postgres');

INSERT INTO parts (part_id, cluster_id)
    VALUES (1, 1), (2, 2), (3, 2);

INSERT INTO priorities (part_id, host_id, conn_id, priority) VALUES
    (1, 1, 1, 0),
    (2, 2, 2, 0),
    (3, 3, 3, 0);
