ALTER TABLE deploy.minions DROP CONSTRAINT check_registered;

ALTER TABLE deploy.minions
    ADD CONSTRAINT check_registered CHECK (
        -- registered
        (register_until IS NOT NULL AND pub_key IS NULL)
        OR
        -- unregisted
        (register_until IS NULL AND pub_key IS NOT NULL)
        OR
        -- allowed to reregister
        (register_until IS NOT NULL AND pub_key IS NOT NULL)
    );
