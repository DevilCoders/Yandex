UPDATE s3.buckets_history
    SET deleted='9999-12-31'
    WHERE deleted IS NULL;
