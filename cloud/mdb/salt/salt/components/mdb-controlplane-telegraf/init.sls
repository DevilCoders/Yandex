{% set use_iam_token_reissuer = salt['pillar.get']('data:solomon:sa_id') %}
include:
    - components.mdb-telegraf
{% if use_iam_token_reissuer %}
    - components.iam-token-reissuer
{% endif %}

extend:
    /etc/telegraf/telegraf.conf:
        file:
            - source: salt://{{ slspath }}/conf/telegraf.conf

/etc/telegraf/yandex_monitoring.token:
{% if use_iam_token_reissuer %}
    file.symlink:
        - target: /etc/iam-token-reissuer/iam-token.txt
{% else %}
    file.managed:
        - contents_pillar: data:solomon:oauth_token
        - user: root
        - group: telegraf
        - mode: '0640'
{% endif %}
        - require:
              - file: /etc/telegraf/telegraf.conf
