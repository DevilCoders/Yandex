# PROXY PRIVATE BILLING
{%- from slspath + "/vars.sls" import config with context -%}

{% set env = grains['yandex-environment'] %}

{% if env == 'testing' %}
  {% set idm_workers_group = [grains["fqdn"]] %}
  {% set cluster = "elliptics-test-proxies" %}
{% else %}
  {% set cluster = "elliptics-proxy" %}
  {% set idm_workers_group = [
  "proxy01iva.mds.yandex.net",
  "proxy02iva.mds.yandex.net",
  "proxy03iva.mds.yandex.net",
  "proxy04iva.mds.yandex.net",
  "proxy05iva.mds.yandex.net",
  "proxy01myt.mds.yandex.net",
  "proxy02myt.mds.yandex.net",
  "proxy03myt.mds.yandex.net",
  "proxy04myt.mds.yandex.net",
  "proxy05myt.mds.yandex.net",
  "proxy01sas.mds.yandex.net",
  "proxy02sas.mds.yandex.net",
  "proxy03sas.mds.yandex.net",
  "proxy04sas.mds.yandex.net",
  "proxy05sas.mds.yandex.net",
  "proxy01vla.mds.yandex.net",
  "proxy02vla.mds.yandex.net",
  "proxy03vla.mds.yandex.net",
  "proxy04vla.mds.yandex.net",
  "proxy05vla.mds.yandex.net",
  "proxy01man.mds.yandex.net",
  "proxy02man.mds.yandex.net",
  "proxy03man.mds.yandex.net",
  "proxy04man.mds.yandex.net",
  "proxy05man.mds.yandex.net"
  ] %}
{% endif %}

{% set s3_mds_proxy_files = [
  "/etc/logrotate.d/s3-dispenser",
  "/etc/logrotate.d/s3-goose",
  "/etc/logrotate.d/s3-goose-private",
  "/etc/logrotate.d/s3-nginx-log",
  "/etc/nginx/conf.d/01-s3-access-log-tskv.conf",
  "/etc/nginx/conf.d/02-resolver.conf",
  "/etc/nginx/include/s3/files.lua",
  "/etc/nginx/include/s3/internal.lua",
  "/etc/nginx/include/s3/lib.lua",
  "/etc/nginx/include/s3/lock.lua",
  "/etc/nginx/include/s3/log.lua",
  "/etc/nginx/include/s3/open_buckets.lua",
  "/etc/nginx/include/s3/private-api.json",
  "/etc/nginx/include/s3/private-api.lua",
  "/etc/nginx/include/s3/shared_dicts.conf",
  "/etc/nginx/include/s3/tools.lua",
  "/etc/nginx/include/s3/values.conf",
  "/etc/nginx/include/s3/values.lua",
  "/etc/nginx/s3/aws_error_pages.conf",
  "/etc/nginx/s3/base_location_options_s3lib.conf",
  "/etc/nginx/s3/bucket_cached_node_24h.conf",
  "/etc/nginx/s3/bucket_cached_node_30m.conf",
  "/etc/nginx/s3/bucket_cached_node_5m.conf",
  "/etc/nginx/s3/bucket_cached_node_5s.conf",
  "/etc/nginx/s3/bucket_cached_node_nirvana.conf",
  "/etc/nginx/s3/bucket_upstream.conf",
  "/etc/nginx/s3/compression_options.conf",
  "/etc/nginx/s3/error_page_503.conf",
  "/etc/nginx/s3/external_cache_options.conf",
  "/etc/nginx/s3/internal_cache_options.conf",
  "/etc/nginx/s3/ping.conf",
  "/etc/nginx/s3/proxy_cache_404_options.conf",
  "/etc/nginx/s3/proxy_cache_options_1m.conf",
  "/etc/nginx/s3/proxy_cache_options_24h.conf",
  "/etc/nginx/s3/proxy_cache_options_30m.conf",
  "/etc/nginx/s3/proxy_cache_options_5m.conf",
  "/etc/nginx/s3/proxy_cache_options_5s.conf",
  "/etc/nginx/s3/proxy_cache_options_nirvana.conf",
  "/etc/nginx/s3/proxy_options.conf",
  "/etc/nginx/s3/s3-system-upstream.conf",
  "/etc/s3-dispenser/s3-dispenser.json",
  "/etc/s3-goose/goose.json",
  "/etc/s3-goose/main-goose.json",
  "/etc/s3-goose/main-private.json",
  "/etc/s3-goose/private.json",
  "/etc/s3-goose/shell.json",
  "/etc/s3-mds-backend/cache-dc-group.json",
  "/etc/ubic/service/s3-goose.json",
  "/usr/share/perl5/Ubic/Service/S3Goose.pm",
  "/usr/share/perl5/Ubic/Service/S3GoosePrivate.pm"
]
%}

  {% if grains["fqdn"] in idm_workers_group %}
  {% set s3_mds_proxy_files_tmp = s3_mds_proxy_files + [
    "/etc/cron.d/s3-dispenser-report-usage",
    "/etc/ubic/service/s3-goose-private.json" ]
  %}
  {%  set s3_mds_proxy_files = s3_mds_proxy_files_tmp %}
  {% endif %}

  {% if env != 'testing' %}
  {% set s3_mds_proxy_files_tmp = s3_mds_proxy_files + [ "/etc/nginx/s3/disabled_url.conf" ] %}
  {% set s3_mds_proxy_files = s3_mds_proxy_files_tmp %}
  {% endif %}

{% set s3_mds_proxy_exec_files = [
   "/etc/init.d/s3-goose",
   "/etc/init.d/s3-goose-private",
   "/usr/local/bin/s3-goose-private.sh" ]
%}

{% set s3_mds_proxy_dirs = [
   "/etc/nginx/s3",
   "/etc/yandex/s3-secret",
   "/var/cache/libmastermind-s3/",
   "/var/cache/s3-goose/",
   "/var/cache/tvm/",
   "/var/log/s3-dispenser/",
   "/var/log/s3-mds-backend/",
   "/var/log/s3/goose-private/",
   "/var/log/s3/goose/" ]
%}

{% set s3_mds_available_files = [
   "/etc/nginx/sites-available/s3-mds-proxy.conf",
   "/etc/nginx/sites-available/s3-mds-idm.conf" ]
%}

{%
  set s3_tvm_secret_testing = pillar['s3_tvm_secret_testing']
%}

{%
  set s3_tvm_secret_prod = pillar['s3_tvm_secret_prod']
%}

{%
  set s3_logbroker_oauth = pillar['s3_logbroker_oauth']
%}


{% if grains["fqdn"] in idm_workers_group %}

s3-goose-private:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker private alive"
    - execution_interval: 300
    - execution_timeout: 30
    - type: s3

{% endif %}

{% for file in s3_mds_proxy_files %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in s3_mds_proxy_exec_files %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in s3_mds_proxy_dirs %}
{{file}}:
  file.directory:
    - mode: 755
    - user: s3proxy
    - group: s3proxy
    - makedirs: True
    - require:
      - user: s3proxy
      - group: s3proxy
{% endfor %}

{% for file in s3_mds_available_files %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - user: root
    - group: root
    - mode: 644
    - template: jinja

{{ file | replace("sites-available", "sites-enabled")}}:
  file.symlink:
    - target: {{ file }}
{% endfor %}

{% set cache_endpoint_set_id = "elliptics-cache-" + grains['yandex-environment'] + ".cache-service.elliptics-cache-1025" %}
{% set cache_remotes = salt['service_discovery.get_endpoints'](cache_endpoint_set_id, grains['conductor']['root_datacenter']) | map('regex_replace', '$', ':10') | list %}
{% if grains['yandex-environment'] == 'testing' and cache_remotes|length > 0 %}
/etc/s3-mds-backend/cache-remotes.json:
  file.serialize:
    - dataset: {{ cache_remotes }}
    - formatter: json
{% else %}
/etc/s3-mds-backend/cache-remotes.json:
  file.serialize:
    - dataset: {{ salt['conductor']['groups2hosts'](cluster, datacenter=grains['conductor']['root_datacenter']) | map('regex_replace', '$', ':1025:10') | list }}
    - formatter: json
{% endif %}

{% for file in [
  '/usr/local/bin/monrun-check-mtime.sh',
] %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

s3-goose:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker proxy alive"
    - execution_interval: 300
    - execution_timeout: 30
    - type: s3

s3-open-buckets-sync:
  monrun.present:
    - command: /usr/local/bin/monrun-check-mtime.sh '/var/tmp/nginx/open_buckets.json' 10 30 2
    - execution_interval: 60
    - execution_timeout: 20
    - type: s3

nginx-maintenance-file:
  monrun.present:
    - command: /usr/local/bin/monrun-check-mtime.sh '/etc/nginx/maintenance.file' 0 0 0
    - execution_interval: 30
    - execution_timeout: 20
    - type: s3

s3-goose-db:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker proxy pg"
    - execution_interval: 120
    - execution_timeout: 30
    - type: s3

s3-goose-db-pool:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker proxy pg_pool"
    - execution_interval: 60
    - execution_timeout: 30
    - type: s3

/usr/local/yasmagent/CONF/agent.s3gooseproxy.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/yasmagent/CONF/agent.s3gooseproxy.conf
    - user: root
    - group: root
    - mode: 644

restart-yasmagent-yasm-s3gooseproxy:
  pkg.installed:
    - pkgs:
      - yandex-yasmagent
  service.running:
    - name: yasmagent
    - enable: True
    - sig: yasmagent
    - watch:
      - file: /usr/local/yasmagent/CONF/agent.s3gooseproxy.conf

# MDS-17322
/usr/local/yasmagent/CONF/agent.s3mdsstat.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/yasmagent/CONF/agent.s3mdsstat.conf
    - user: root
    - group: root
    - mode: 644

restart-yasmagent-yasm-s3mdsstat:
  pkg.installed:
    - pkgs:
      - yandex-yasmagent
  service.running:
    - name: yasmagent
    - enable: True
    - sig: yasmagent
    - watch:
      - file: /usr/local/yasmagent/CONF/agent.s3mdsstat.conf


{% set components = [
    "goose",
    "goose-private"
] %}

{% for comp in components %}
libmds-{{ comp }}:
  yafile.managed:
    - name: /etc/s3-goose/libmds-{{ comp }}.json
    - source: salt://{{ slspath }}/files/etc/s3-goose/libmds.json
    - template: jinja
    - defaults:
        component_name: "{{ comp }}"
{% endfor %}
