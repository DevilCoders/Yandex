CREATE TABLE s3.clouds
(
    cloud_id        text NOT NULL,
    status          int DEFAULT 0,
    max_size        bigint,
    max_buckets     int,

    CONSTRAINT pk_clouds_id PRIMARY KEY (cloud_id)
);

ALTER TABLE ONLY s3.accounts 
    ADD COLUMN max_buckets      bigint,
    ADD COLUMN cloud_id         text,
    ADD COLUMN folder_id        text,
    ADD CONSTRAINT fk_account_cloud_id
        FOREIGN KEY (cloud_id) REFERENCES s3.clouds ON DELETE RESTRICT;

UPDATE ONLY s3.accounts
SET
    folder_id = cast(service_id as text)
WHERE folder_id IS NULL;

CREATE SEQUENCE s3.auto_account_id START WITH 9000000;

CREATE INDEX idx_account_cloud_id ON s3.accounts USING btree (cloud_id) WHERE cloud_id IS NOT NULL;
ALTER TABLE s3.accounts ADD CONSTRAINT unq_account_folder UNIQUE (folder_id);
