/etc/network/interfaces:
    file.managed:
        - template: jinja
{% if salt['pillar.get']('data:network:interfaces', False) %}
        - contents_pillar: data:network:interfaces
{% else %}
        - source: salt://{{ slspath }}/conf/interfaces
{% endif %}
        - mode: 644
