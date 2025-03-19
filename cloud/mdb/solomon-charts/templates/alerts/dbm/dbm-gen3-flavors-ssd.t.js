{% for dc in dc_lst_gen3 %}
let << dc >> = {project='<< project_id >>', cluster='<< dbm_attrs.get("cluster")>>', service='<< dbm_attrs.get("service")>>', geo='<<dc>>', gen='3|4', sensor='dbaas_dbm_pgaas_ssd_flavors_s_16vcpu_64ram_2048ssd', node='primary', host='by_node'};
let << dc >>_last = last(series_sum(<< dc >>));
{% endfor%}

{% for dc in dc_lst_gen3 %}
alarm_if(last(series_sum(<<dc>>)) < 16);
{% endfor %}

{% for dc in dc_lst_gen3 %}
warn_if(last(series_sum(<<dc>>)) < 48);
{% endfor %}