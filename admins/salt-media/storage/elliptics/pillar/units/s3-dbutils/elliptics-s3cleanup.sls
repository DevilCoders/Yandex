include:
    - units.s3-dbutils.common

s3-dbutils:
    monrun:
        pg_counters_queue: 
            warn: 1000000
            crit: 3000000
            chunk_size_threshold: 300000
        pg_counters_update:
            warn: 4000
            crit: 9000
    scripts:
        update_chunks_counters:
            batch_size: 2000000
        update_chunks_counters_meta:
            threads: 5
        chunk_merger:
            exclude_bids: "d61cd0c4-4b65-4aad-88ec-6c6aef6e98bf"
        chunk_mover:
            threads: 11
    solomon:
        cluster: "production" 
    secrets: 
        s3util: "s3util-password"
        s3api_ro: "s3-idm-s3int-db-password-ro"