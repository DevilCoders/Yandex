let s_total =  group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< py_int_api.get("cluster") >>',
        service='<< py_int_api.get("service") >>',
        host='<< py_int_api.get("host_mask") >>',
        sensor='int_api_dbaas_internal_api_count_*xx_dmmm_rate',
        node='replica',
        dc='*'
    }
    );

let s_5xx =  group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< py_int_api.get("cluster") >>',
        service='<< py_int_api.get("service") >>',
        host='<< py_int_api.get("host_mask") >>',
        sensor='int_api_dbaas_internal_api_count_5xx_dmmm_rate',
        node='replica',
        dc='*'
    }
    );

let rate = s_5xx/s_total;
let rate_exp = avg(s_5xx/s_total);
let is_yellow = rate_exp >= 0.005;
let is_red = rate_exp >= 0.01;
let trafficColor = is_red ? 'red' : (is_yellow ? 'yellow' : 'green');

alarm_if(is_red);
warn_if(is_yellow);
