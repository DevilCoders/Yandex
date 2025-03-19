let p80 = histogram_percentile(99,
    {
        project='<< project_id >>',
        cluster='<< py_int_api.get("cluster") >>',
        service='<< py_int_api.get("service") >>',
        app='<< py_int_api.get("app") >>',
        name='http_request_duration_seconds'
    }
);

let crit_threshold_milliseconds = 2000;
let warn_threshold_milliseconds = 1000;

let avg = avg(p80);

alarm_if(avg > crit_threshold_milliseconds);
warn_if(avg > warn_threshold_milliseconds);
