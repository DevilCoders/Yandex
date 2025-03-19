{% set cluster = pillar.get('cluster') %}
{% set unit = 'elliptics-cache' %}

/etc/elliptics/elliptics.conf:
  file.managed:
    - source: salt://units/{{ unit }}/files/etc/elliptics/elliptics.conf
    - template: jinja

elliptics-cache-dependencies:
  pkg.installed:
    - name: elliptics
    - name: config-elliptics-node
    - name: yandex-elliptics-cache-scripts

{% set dc_hosts = salt['conductor']['groups2hosts'](cluster, datacenter=grains['conductor']['root_datacenter']) | list %}
/etc/elliptics/masters.id:
  file.managed:
    - contents: |
        {%- for host in dc_hosts %}
        {{ host }}
        {%- endfor %}
    - user: root
    - group: root
    - mode: 644

{% set cache_endpoint_set_id = "elliptics-cache-" + grains['yandex-environment'] + ".cache-service.elliptics-cache-1025" %}
{% set cache_remotes = salt['service_discovery.get_endpoints'](cache_endpoint_set_id, grains['conductor']['root_datacenter']) | list %}
{% if grains['yandex-environment'] in ['testing','prestable'] and cache_remotes|length > 0 %}
/etc/elliptics/cache-remotes.id:
  file.managed:
    - contents: 
        {% for remote in cache_remotes %}
        {{ remote }}:10
        {% endfor %}
{% else %}
/etc/elliptics/cache-remotes.id:
  file.managed:
    - contents: |
        {%- for host in dc_hosts %}
        {{ host }}:1025:10
        {%- endfor %}
    - user: root
    - group: root
    - mode: 644
{% endif %}

### S3 cache
/srv/storage/1/1/kdb:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755
    - makedirs: True

/srv/storage/1/1/kdb/group.id:
  file.managed:
    - source: salt://units/{{ unit }}/files/srv/storage/1/1/kdb/group.id
    - template: jinja

gen_ids_id:
  cmd.run:
    - name: dd if=/dev/urandom of=/srv/storage/1/1/kdb/ids bs=8 count=64
    - unless: 'if [ -f /srv/storage/1/1/kdb/ids ]; then exit 0; else exit 1; fi'

{% set s3_cache_size = 32212254720 -%}
{%- if grains['mem_total'] <= 67584 -%}
{%- set s3_cache_size = s3_cache_size / 2 -%}
{%- endif %}
/srv/storage/1/1/kdb/device.conf:
  file.managed:
    - contents: |
        cache_size = {{ s3_cache_size | int }}
    - user: root
    - group: root
    - mode: 644
### S3 cache

### mediastorage
### TODO: Upgrade 64G hosts
{% for group_id, group_params in pillar.get('elliptics-cache', {}).items() %}
{% set group_path = group_params['path'] %}
{% set cache_size = group_params['size'] -%}
{%- if grains['mem_total'] <= 67584 -%}
{%- set cache_size = cache_size / 2 -%}
{%- endif %}
{{ group_path }}/kdb:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755
    - makedirs: True

{{ group_path }}/kdb/group.id:
  file.managed:
    - contents: |
        {{ group_id }}
    - user: root
    - group: root
    - mode: 644

{{ group_path }}.gen_ids_id:
  cmd.run:
    - name: dd if=/dev/urandom of={{ group_path }}/kdb/ids bs=8 count=64
    - unless: 'if [ -f {{ group_path }}/kdb/ids ]; then exit 0; else exit 1; fi'

{{ group_path }}/kdb/device.conf:
  file.managed:
    - contents: |
        cache_size = {{ cache_size | int }}
    - user: root
    - group: root
    - mode: 644
{% endfor %}
### mediastorage
