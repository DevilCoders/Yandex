include:
{% if salt['pillar.get']('data:use_yasmagent', True) %}
    - .enabled
{% else %}
    - .absent
{% endif %}
    - .external

yasmagent-default-instance-getter:
    file.managed:
        - name: /usr/local/yasmagent/default_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/default.getter.py
        - makedirs: True
        - mode: 755
        - defaults:
            instances: {{ salt['pillar.get']('data:yasmagent:instances', ['']) | join(',') }}
