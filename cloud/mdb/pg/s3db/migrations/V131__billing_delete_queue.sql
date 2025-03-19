CREATE TABLE s3.billing_delete_queue
(
    bid             uuid             NOT NULL,
    name            text COLLATE "C" NOT NULL,
    part_id         integer,
    data_size       bigint           NOT NULL DEFAULT 0,
    deleted_ts      timestamptz      NOT NULL,
    created         timestamptz      NOT NULL,
    storage_class   integer          NOT NULL,
    status          integer          NOT NULL DEFAULT 0
);

CREATE UNIQUE INDEX pk_billing_delete_queue ON s3.billing_delete_queue (bid, name, COALESCE(part_id, 0), created);
CREATE INDEX i_billing_delete_queue_ts ON s3.billing_delete_queue USING btree (status, deleted_ts);
