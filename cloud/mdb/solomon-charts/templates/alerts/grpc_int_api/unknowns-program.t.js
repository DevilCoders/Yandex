let s_total = drop_empty_lines(group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< grpc_int_api.get("cluster") >>',
        service='mdb_tgf',
        host!='total', 
        DC!='total', 
        name='prometheus.grpc_server_handled_total',
        grpc_code='total'
    }
));
let s_unknown = drop_empty_lines(group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< grpc_int_api.get("cluster") >>',
        service='mdb_tgf',
        host!='total', 
        DC!='total', 
        name='prometheus.grpc_server_handled_total',
        grpc_code='Unknown|Aborted|Internal|DataLoss'
    }
));

no_data_if(size(s_total) == 0);
no_data_if(size(s_unknown) == 0);

let rate = avg(100 * s_unknown/s_total);
let count = sum(s_unknown);

let str_rate = to_fixed(rate, 2);
let str_count = to_fixed(count, 0);

// Don't alarm on single Unknown (1-2 in a window)
alarm_if(count >= 3.0 && rate >= 1.0);
warn_if(count >= 1.0);
