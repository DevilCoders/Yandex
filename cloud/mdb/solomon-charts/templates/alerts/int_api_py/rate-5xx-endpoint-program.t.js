let s_total =  drop_empty_lines(group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< py_int_api.get("cluster") >>',
        service='<< py_int_api.get("service") >>',
        host='<< py_int_api.get("host_mask") >>',
        sensor='int_api_dbaas_internal_api_<< py_intapi_endpoint >>_count_total_dmmm_rate',
        node='replica',
        dc='*'
    }
));

let s_5xx =  drop_empty_lines(group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< py_int_api.get("cluster") >>',
        service='<< py_int_api.get("service") >>',
        host='<< py_int_api.get("host_mask") >>',
        sensor='int_api_dbaas_internal_api_<< py_intapi_endpoint >>_count_5xx_dmmm_rate',
        node='replica',
        dc='*'
    }
));

no_data_if(size(s_total) == 0);
no_data_if(size(s_5xx) == 0);

let rate = avg(100 * s_5xx/s_total);
let count = 15 * sum(s_5xx); // we send metrics every 15 second, so rate is 1/15

let str_rate = to_fixed(rate, 2);
let str_count = to_fixed(count, 0);

// Don't alarm on single 5xx (1-2 in a window)
alarm_if(count >= 3.0 && rate >= 1.0);
warn_if(count >= 1.0);
