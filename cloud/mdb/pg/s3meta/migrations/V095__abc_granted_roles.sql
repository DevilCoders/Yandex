ALTER TABLE s3.granted_roles
    ADD COLUMN is_idm boolean DEFAULT false,
    ADD COLUMN is_abc boolean DEFAULT false
;

UPDATE s3.granted_roles SET is_idm = true WHERE role IS NOT NULL;

CREATE OR REPLACE FUNCTION s3.update_ts() RETURNS TRIGGER
    LANGUAGE plpgsql
AS $$
BEGIN
    NEW.issue_date = current_timestamp;

    RETURN NEW;
END;
$$;

CREATE TRIGGER granted_roles_update_ts
    BEFORE UPDATE ON s3.granted_roles
    FOR EACH ROW
EXECUTE PROCEDURE s3.update_ts();
