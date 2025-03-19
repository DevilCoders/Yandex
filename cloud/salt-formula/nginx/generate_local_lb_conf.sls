{%- set endpoints = grains['cluster_map']['load_balancer']['endpoints'] -%}

{%- for service_name in endpoints %}
  {%- if service_name in upstream_ports and endpoints[service_name]['host'] != 'localhost' %}
    {%- if endpoints[service_name].get('reals', []) %}
upstream {{ service_name }} {
  hash $hostname;
  {%- for fqdn in endpoints[service_name].get('reals', []) %}
  server {{ fqdn }}:{{ upstream_ports[service_name] }};
  {%- endfor %}
}

server {
  listen [::1]:{{ endpoints[service_name]['port'] }};
  proxy_pass {{ service_name }};
  proxy_connect_timeout 1s;
  access_log /var/log/nginx/{{ service_name }}-local-lb_access.log stream;
  error_log /var/log/nginx/{{ service_name }}-local-lb_error.log;
}
    {%- endif %}
  {%- endif %}
{%- endfor %}
