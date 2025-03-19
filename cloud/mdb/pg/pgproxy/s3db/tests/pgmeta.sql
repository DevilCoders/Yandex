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
    (2, 's3meta01r', 'LOCAL', 20),
    (3, 's3meta02', 'LOCAL', 0),
    (4, 's3meta02r', 'LOCAL', 20),
    (5, 's3db01', 'LOCAL', 0),
    (6, 's3db01r', 'LOCAL', 20),
    (7, 's3db02', 'LOCAL', 0),
    (8, 's3db02r', 'LOCAL', 20),
    (9, 's3db01s', 'OTHER', 30),
    (10,'s3db02d', 'OTHER', 100),
    (11,'s3db02s', 'OTHER', 30);

INSERT INTO connections (conn_id, conn_string) VALUES
    (1, 'host=s3meta01 dbname=s3meta user=postgres'),
    (2, 'host=s3meta01r user=postgres dbname=s3meta'),
    (3, 'host=s3meta02 dbname=s3meta user=postgres port=5432'),
    (4, 'host=s3meta02r port=5432  dbname=s3meta user=postgres'),
    (5, 'host=s3db01 dbname=s3db user=postgres'),
    (6, 'host=s3db01r dbname=s3db user=postgres'),
    (7, 'host=s3db02 dbname=s3db user=postgres'),
    (8, 'host=s3db02r dbname=s3db user=postgres'),
    (9, 'host=s3db01s dbname=s3db user=postgres'),
    (10,'host=s3db02d dbname=s3db user=postgres'),
    (11,'host=s3db02s dbname=s3db user=postgres');

INSERT INTO parts (part_id, cluster_id)
    VALUES (1, 1), (2, 1), (3, 2), (4, 2);

INSERT INTO priorities (part_id, host_id, conn_id, priority) VALUES
    (1, 1, 1, 0),
    (1, 2, 2, 20),
    (2, 3, 3, 0),
    (2, 4, 4, 20),
    (3, 5, 5, 0),
    (3, 6, 6, 20),
    (4, 7, 7, 0),
    (4, 8, 8, 20),
    (3, 9, 9, 30),
    (4, 10,10,100),
    (4, 11,11,30);
