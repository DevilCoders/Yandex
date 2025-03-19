{% set target_container = salt['pillar.get']('target-container') %}

images-ready:
    test.nop

{% if target_container %}

{% if not salt['pillar.get']('is-deleting') and '.' in target_container and target_container not in salt['pillar.get']('data:porto', {}) %}
target-container-missing-in-pillar:
    test.configurable_test_state:
        - result: False
        - comment: 'Target container {{ target_container }} not found in pillar'
{% endif %}

{% if salt['pillar.get']('is-deleting') and not salt['pillar.get']('data:porto:' + target_container + ':container_options:pending_delete') %}
# MDB-10649: Sometimes, we got not actual pillar version (probably cached).
# It's better to fail in such cases. Overwise (in case of container delete), we may have orphan containers.
pending-delete-not-set-for-target-container:
    test.configurable_test_state:
        - result: False
        - comment: 'Target container {{ target_container }} pending_delete not set, but is-deleting flag is set'
{% endif %}

{% endif %}

/var/cache/porto_agent_states:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 0700

{% set huge_pages = [0] %}
{% for container in salt['pillar.get']('data:porto', {}).keys() %}
{%   set limit_bytes = salt['pillar.get']('data:porto:' + container + ':container_options:hugetlb_limit', 0)|int %}
{%   do huge_pages.append(huge_pages.pop() + (limit_bytes / 2048 / 1024)|int) %}
{%   if not target_container or container == target_container %}

porto_agent_state_{{ container }}:
    file.managed:
        - name: /var/cache/porto_agent_states/{{ container }}.json
        - template: jinja
        - source: salt://{{ slspath }}/conf/container_agent_state.json
        - defaults:
            container: {{ container }}
        - require:
          - file: /var/cache/porto_agent_states

{% if salt['pillar.get']('update-containers', True) %}
porto_agent_update_{{ container }}:
    porto_agent.run:
        - command: update
        - container: {{ container }}
        - flags: ['--loglevel', 'Info', '--logshowall']
        - secrets: {{ salt['pillar.get']('data:porto:' + container + ':container_options:secrets', {}) | tojson }}
        - require:
          - porto_agent_state_{{ container }}
{% endif %}

{%   endif %}
{% endfor %}

{% if salt['pillar.get']('data:sysctl:vm.nr_hugepages', -1) != -1 %}
{%     do huge_pages.pop() %}
{%     do huge_pages.append(salt['pillar.get']('data:sysctl:vm.nr_hugepages')) %}
{% endif %}
{% set total_ram_pages = salt['grains.get']('mem_total', 128000) / 2 %}
{% set total_huge_pages = huge_pages[0] | int %}
{#
       Reserve 1% for historical reasons (need to debug)
#}
{% if total_huge_pages > 0 %}
{%     set total_huge_pages = (total_huge_pages + 0.01 * total_ram_pages)|int %}
{% endif %}

vm.nr_hugepages:
    sysctl.present:
        - value: {{ total_huge_pages }}
        - config: /etc/sysctl.d/hugepages.conf

{% set max_map_count = salt['pillar.get']('data:sysctl:vm.max_map_count', -1) | int %}
{% if max_map_count != -1 %}
vm.max_map_count:
    sysctl.present:
        - value: {{ max_map_count }}
        - config: /etc/sysctl.d/max_map_count.conf
{% endif %}
