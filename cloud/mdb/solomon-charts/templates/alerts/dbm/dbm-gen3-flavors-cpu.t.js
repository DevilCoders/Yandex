{% for dc in dc_lst_gen3 %}
let << dc >> = {project='<< project_id >>', cluster='<< dbm_attrs.get("cluster")>>', service='<< dbm_attrs.get("service")>>', geo='<<dc>>', gen='3|4', sensor='dbaas_dbm_pgaas_free_cores', node='primary', host='by_node'};
let << dc >>_last = last(series_sum(<< dc >>));
{% endfor%}

{% for dc in dc_lst_gen3 %}
alarm_if(last(series_sum(<<dc>>)) < 500);
{% endfor %}

{% for dc in dc_lst_gen3 %}
warn_if(last(series_sum(<<dc>>)) < 2000);
{% endfor %}