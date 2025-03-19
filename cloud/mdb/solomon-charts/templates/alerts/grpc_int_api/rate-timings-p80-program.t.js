let p80 = histogram_percentile(99,
    {
        project='<< project_id >>',
        cluster='<< grpc_int_api.get("cluster") >>',
        service='<< grpc_int_api.get("service") >>',
        app='<< grpc_int_api.get("app") >>',
        name='grpc_server_handling_seconds'
    }
);

let crit_threshold_milliseconds = 2000;
let warn_threshold_milliseconds = 1000;

let avg = avg(p80);

alarm_if(avg > crit_threshold_milliseconds);
warn_if(avg > warn_threshold_milliseconds);
