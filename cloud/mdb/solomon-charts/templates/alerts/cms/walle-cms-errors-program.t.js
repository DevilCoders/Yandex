let s_total =  drop_empty_lines(group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< cms.get("api_cluster") >>',
        service='mdb',
        host='by_node',
        sensor='mdb_cms_api_requests_total_code_*_method_*_ammv_rate'
    }
));

let s_5xx =  drop_empty_lines(group_lines(
    'sum',
    {
        project='<< project_id >>',
        cluster='<< cms.get("api_cluster") >>',
        service='mdb',
        host='by_node',
        sensor='mdb_cms_api_requests_total_code_500_method_*_ammv_rate'
    }
));

no_data_if(size(s_total) == 0);
no_data_if(size(s_5xx) == 0);

let rate = 100 * s_5xx/s_total;
let value = avg(rate);
let str_value = to_fixed(value, 2);

alarm_if(value >= 1.0);
warn_if(value >= 0.5);
