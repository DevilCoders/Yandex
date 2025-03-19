INSERT INTO billing.tracks (cluster_id,
                            cluster_type,
                            bill_type,
                            from_ts,
                            until_ts,
                            updated_at)
VALUES (%(cluster_id)s,
        %(cluster_type)s,
        %(bill_type)s,
        now(),
        NULL,
        NULL)
ON CONFLICT (cluster_id, bill_type) DO UPDATE SET from_ts = now()
