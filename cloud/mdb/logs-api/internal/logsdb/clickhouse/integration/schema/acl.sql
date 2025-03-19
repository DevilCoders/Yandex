--///////////////////////////////////////////////////////
--//              Users and ACL                        //
--///////////////////////////////////////////////////////

-- User for DataTransfer
CREATE USER IF NOT EXISTS mdb_logs_writer ON CLUSTER '{cluster}' IDENTIFIED WITH SHA256_PASSWORD BY 'PASSWORD';
GRANT ON CLUSTER '{cluster}' SELECT, UPDATE, CREATE, DROP, SHOW, INSERT ON mdb.* TO mdb_logs_writer;
GRANT ON CLUSTER '{cluster}' SELECT ON system.* TO mdb_logs_writer;

-- User for logs API
CREATE USER IF NOT EXISTS mdb_logs_reader ON CLUSTER '{cluster}' IDENTIFIED WITH SHA256_PASSWORD BY 'PASSWORD';
GRANT ON CLUSTER '{cluster}' SELECT ON mdb.* TO mdb_logs_reader;
