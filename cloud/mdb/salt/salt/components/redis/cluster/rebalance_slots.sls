rebalance_slots:
    mdb_redis.rebalance_slots


{{ salt.mdb_metrics.get_redis_no_slots_marker_filename() }}:
    file.absent
