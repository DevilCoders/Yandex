ALTER TABLE deploy.minions
  ADD COLUMN deleted boolean NOT NULL DEFAULT false;

CREATE OR REPLACE VIEW deploy.minions_v1 (minion_id, fqdn, group_id, master_id, pub_key, auto_reassign, created_at, updated_at, register_until, deleted) AS
  SELECT minion_id, fqdn, group_id, master_id, pub_key, auto_reassign, created_at, updated_at, register_until, deleted
    FROM deploy.minions;
