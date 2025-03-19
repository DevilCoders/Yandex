include:
    - units.s3-dbutils.common

s3-dbutils:
    monrun:
        pg_counters_queue: 
            warn: 50000
            crit: 500000
            chunk_size_threshold: 90000
        pg_counters_update:
            warn: 1000
            crit: 3000
    scripts:
        update_chunks_counters:
            batch_size: 50000
        update_chunks_counters_meta:
            threads: 2
        chunk_mover:
            threshold: 200000
            max-objects: 150000
            min-objects: 5000
            delay: "1 day"
            auto: True
    solomon:
        cluster: "testing" 
    secrets: 
        s3util: "s3util-password-testing"
        s3api_ro: "s3-idm-s3int-db-password-testing-ro"


        