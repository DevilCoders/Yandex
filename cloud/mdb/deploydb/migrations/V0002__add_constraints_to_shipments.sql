ALTER TABLE deploy.shipments
    ADD CONSTRAINT check_batch_size CHECK (
        batch_size > 0
    );

ALTER TABLE deploy.shipments
    ADD CONSTRAINT check_stop_on_error_count CHECK (
        stop_on_error_count >= 0 AND stop_on_error_count <= total_count
    );


ALTER TABLE deploy.shipments
    ADD CONSTRAINT check_fqdns CHECK (
        array_length(fqdns, 1) > 0
    );
