{% for project in  g.env_ctx.project_lst %}
let <<project>>_a = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', host='*-rc1a', sensor='docs_in_marketplace_index_<<project>>_axxx'});
let <<project>>_a_count = last(<<project>>_a);

let <<project>>_b = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', host='*-rc1b', sensor='docs_in_marketplace_index_<<project>>_axxx'});
let <<project>>_b_count = last(<<project>>_b);

let <<project>>_c = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', host='*-rc1c', sensor='docs_in_marketplace_index_<<project>>_axxx'});
let <<project>>_c_count = last(<<project>>_c);
{% endfor %}

let sum_a = 0
{% for project in  g.env_ctx.project_lst %}    
    + <<project>>_a_count
{% endfor %}
;

let total_a = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', host='*-rc1a', sensor='docs_in_marketplace_index_*_axxx'});
let total_b = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', host='*-rc1b', sensor='docs_in_marketplace_index_*_axxx'});
let total_c = group_lines('sum', {project='<< project_id >>', cluster='<< indexer.get("cluster") >>', service='yc-search-stat', host='*-rc1c', sensor='docs_in_marketplace_index_*_axxx'});


let total_a_count = last(total_a);
let total_b_count = last(total_b);
let total_c_count = last(total_c);

let total_equal = (total_a_count == total_b_count) && (total_b_count == total_c_count);
let sum_equals_total = sum_a == total_a_count;


let is_not_alarm = sum_equals_total && total_equal
{% for project in  g.env_ctx.project_lst %}    
    && (<<project>>_a_count == <<project>>_b_count) && (<<project>>_b_count == <<project>>_c_count)
{% endfor %}
;

alarm_if(!is_not_alarm);

