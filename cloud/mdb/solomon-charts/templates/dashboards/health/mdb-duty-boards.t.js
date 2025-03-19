{%- for engine in engines -%}[<< g.db_ctxs[engine].name >> board](<< solomon_url >>?project=<< project_id >>&cluster=<< health.get("cluster") >>&service=<< health.get("service") >>&dashboard=<< id_prefix >>-health-<< engine >>-duty-board)
{%- if not loop.last -%}&nbsp;&nbsp;{%- endif -%}
{% endfor %}