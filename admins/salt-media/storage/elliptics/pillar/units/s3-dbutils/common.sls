s3-dbutils:
    dirs:
        status_dir: "/etc/s3-dbutils/statuses"
        log_dir: "/var/log/s3/dbutils"
        lock_dir: "/etc/s3-dbutils/locks"
    solomon:
        project: "s3-dbutils"
        service: "nodes"
    postgres:
        goose_url: "http://localhost:3355/stats/postgres"
        pgpass_file: "/etc/s3-dbutils/.pgpass"
        port: 6432
        ro_user: "s3api_ro"
        rw_user: "s3util"
        mon_user: "s3api_ro"
        pgmeta:
            conf_file: "/etc/s3-goose/postgres.json"
    scripts:
        update_buckets_usage:
            threads: 8
        check_chunks_counters:
            critical_errors_filename: "critical_errors.log" 
        check_chunks_counters_meta:
            critical_errors_filename: "meta_critical_errors.log"
        default_timeout: 3600
    monrun:
        status_file_old_threshold: 3700
        status_file_checks:
            - name: "update_chunks_counters"
            - name: "update_chunks_counters_meta"
            - name: "update_bucket_stat"
            - name: "update_shard_stat"
            - name: "update_buckets_usage"
            - name: "update_buckets_size"
              threshold: 610000
            - name: "check_chunks_counters"
              threshold: 610000
            - name: "check_chunks_counters_meta"
              threshold: 610000
