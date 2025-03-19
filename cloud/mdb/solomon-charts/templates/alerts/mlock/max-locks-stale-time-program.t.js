let stale_time = max(series_max(
    {
        project='<< project_id >>',
        cluster='<< common_labels.get("cluster") >>',
        service='<< common_labels.get("service") >>',
        namespace='<< common_labels.get("namespace") >>',
        app='mlock',
        name='mlock_max_locks_stale_time'
    }
));

let warn_threshold_seconds = 7 * 24 * 3600;
let crit_threshold_seconds = 14 * 24 * 3600;

alarm_if(stale_time > crit_threshold_seconds);
warn_if(stale_time > warn_threshold_seconds);
