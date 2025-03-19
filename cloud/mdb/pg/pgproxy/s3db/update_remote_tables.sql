CREATE OR REPLACE FUNCTION plproxy.update_remote_tables()
RETURNS integer
LANGUAGE plpgsql
AS $$
BEGIN
    CREATE EXTENSION IF NOT EXISTS postgres_fdw;
--start of the section for FDW

--end of the section for FDW

    IMPORT FOREIGN SCHEMA public
        LIMIT TO (clusters, parts, hosts, connections, priorities, config)
        FROM SERVER remote
        INTO public;

    DROP TABLE IF EXISTS plproxy.clusters;
    CREATE TABLE plproxy.clusters (LIKE clusters);
    INSERT INTO plproxy.clusters SELECT * FROM clusters;

    DROP TABLE IF EXISTS plproxy.parts;
    CREATE TABLE plproxy.parts (LIKE parts);
    INSERT INTO plproxy.parts SELECT * FROM parts;

    DROP TABLE IF EXISTS plproxy.hosts;
    CREATE TABLE plproxy.hosts (LIKE hosts);
    INSERT INTO plproxy.hosts SELECT * FROM hosts;

    DROP TABLE IF EXISTS plproxy.connections;
    CREATE TABLE plproxy.connections (LIKE connections);
    INSERT INTO plproxy.connections SELECT * FROM connections;

    DROP TABLE IF EXISTS plproxy.priorities;
    CREATE TABLE plproxy.priorities (LIKE priorities);
    CREATE TRIGGER update_cluster_version AFTER INSERT OR DELETE OR UPDATE
        ON plproxy.priorities
        FOR EACH STATEMENT
        EXECUTE PROCEDURE plproxy.inc_cluster_version();
    INSERT INTO plproxy.priorities SELECT * FROM priorities;

    DROP TABLE IF EXISTS plproxy.config;
    CREATE TABLE plproxy.config (LIKE config);
    INSERT INTO plproxy.config SELECT * FROM config;

--start of the section for grants

--end of the section for grants

    UPDATE plproxy.versions SET version = version + 1;

    DROP SERVER remote CASCADE;
    RETURN 0;
END;
$$;
