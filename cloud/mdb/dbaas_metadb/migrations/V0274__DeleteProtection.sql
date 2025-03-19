ALTER TABLE dbaas.clusters ADD COLUMN deletion_protection bool NOT NULL DEFAULT FALSE;

CREATE OR REPLACE FUNCTION dbaas.deletion_protection_check() RETURNS TRIGGER
              LANGUAGE plpgsql AS
              $function$
BEGIN
    IF OLD.deletion_protection THEN
        RAISE EXCEPTION 'Deletion of cluster % prohibited due to delete protection', OLD.cid;
    END IF;
    RETURN OLD;
END;
$function$;

CREATE TRIGGER tg_deletion_protection_check BEFORE DELETE ON dbaas.clusters
    FOR EACH ROW EXECUTE PROCEDURE dbaas.deletion_protection_check();
