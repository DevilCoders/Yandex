{% set cluster = pillar.get('cluster') %}
{% set unit = 'nginx' %}

include:
  - templates.certificates

{% if pillar.get('nginx-files') != None %}
{% for file in pillar.get('nginx-files') %}
{{file}}:
  yafile.managed:
    - source: salt://files/{{ cluster }}{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}
{% endif %}

{% for file in pillar.get('nginx-monrun-files') %}
{{file}}:
  yafile.managed:
    - source: salt://files/{{ cluster }}{{ file }}
    - mode: 644 
    - user: root
    - group: root
    - makedirs: True
    - watch_in: monrun-regenerate
{% endfor %}

{% if pillar.get('nginx-config-files') != None %}
{% for file in pillar.get('nginx-config-files') %}
{{file}}:
  yafile.managed:
    - source: salt://files/{{ cluster }}{{ file }}
    - template: jinja
    - mode: 644 
    - user: root
    - group: root
    - makedirs: True
    - watch_in:
      - service: nginx
{% endfor %}
{% endif %}

{% for file in pillar.get('nginx-dirs',[]) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
{% endfor %}

/etc/nginx/ssl/certuml4.pem:
  file.managed:
    - contents_pillar: yav:certuml4.pem
    - makedirs: True
    - user: root
    - group: root
    - mode: 0400
    - require_in:
      - service: nginx

/etc/yandex-certs/allCAs.pem:
  file.managed:
    - contents_pillar: yav:allCAs.pem
    - makedirs: True
    - user: root
    - group: root
    - mode: 0400
    - require_in:
      - service: nginx

# avatars is mostly limited by CPU
# we assume that each 1G of network should give net rs weight of 1 to the host
# and 32 CPU cores should give cpu weight 10
# Host RS weight is calculated as minimal of net and cpu weights

# Some CPU models are more effective per-core than others. We score one core of 6230 as approx. 1.5 cores of 2650v2
{%- set cpu_model_weight = {"Gold 6230 CPU": 40, "Gold 6230R CPU": 52} %}

{%- if grains['yandex-environment'] == 'testing' %}
    {%- set slb_weight = { "weight": 1 } %}
{%- else %}
    {%- set slb_weight = { "weight": ( grains['num_cpus'] | int * 10 / 32 )} %}
    {%- for model, weight in cpu_model_weight.items() %}
        {%- if model in grains['cpu_model'] %}
            {%- if slb_weight.update({"weight": weight}) %}{%- endif %}
        {%- endif %}
    {%- endfor %}
    {%- set net_speed = salt['cmd.shell']('source /usr/local/sbin/autodetect_active_eth; ethtool $default_route_iface |grep Speed |awk "{print $NF}"  |egrep "[0-9]+" -o') | int %}

    {%- if net_speed > 0 %}
        {%- set net_weight = ( net_speed / 1000 * 2 ) %}
    {%- else %}
        {%- set net_weight = 1 %}
    {%- endif %}
    {%- if net_weight < slb_weight["weight"] %}
        {%- if slb_weight.update({"weight": net_weight}) %}{%- endif %}
    {%- endif %}
{%- endif %}

/etc/nginx/include/rs-weight.conf:
  file.managed:
    - source: salt://files/{{cluster}}/etc/nginx/include/rs-weight.conf
    - template: jinja
    - context:
        slb_weight: {{slb_weight["weight"] | int}}
    - watch_in:
      - service: nginx
