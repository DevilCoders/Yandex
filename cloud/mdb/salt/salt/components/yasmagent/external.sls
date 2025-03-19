yasmagent-suppressor:
{% if salt['pillar.get']('data:suppress_external_yasmagent', False) %}
    file.managed:
        - name: /etc/yandex/suppress-external-yasmagent
        - order: 2
        - contents: ''
{% else %}
    file.absent:
        - name: /etc/yandex/suppress-external-yasmagent
        - order: 2
{% endif %}
