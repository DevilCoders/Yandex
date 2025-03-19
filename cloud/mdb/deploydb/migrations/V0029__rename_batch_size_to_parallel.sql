ALTER TABLE deploy.shipments DROP CONSTRAINT check_batch_size;

ALTER TABLE deploy.shipments RENAME COLUMN batch_size TO parallel;

ALTER TABLE deploy.shipments
    ADD CONSTRAINT check_parallel CHECK (
            parallel > 0
        );
