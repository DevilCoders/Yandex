{%- set federation = pillar.get('mds_federation', None) -%}

{% if federation -%}
/etc/elliptics/federation:
  file.managed:
    - contents: |
        {{ federation }}
    - user: root
    - group: root
    - mode: 644

/etc/elliptics/masters.id-federation-{{ federation }}:
  file.managed:
    - contents: |
        {%- for host in pillar['federations'][federation]['remotes'] %}
        {{ host }}
        {%- endfor %}
    - user: root
    - group: root
    - mode: 644
{%- endif %}

/usr/bin/add_federation_tag.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/add_federation_tag.sh
    - user: root
    - group: root
    - mode: 755

