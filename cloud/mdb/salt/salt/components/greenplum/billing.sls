{% set log_dir = '/var/log/dbaas-billing' %}
{% set shipping_enabled = salt['pillar.get']('data:billing:ship_logs', True) %}
{% set use_cloud_logbroker = salt['pillar.get']('data:billing:use_cloud_logbroker', False) %}

{% if shipping_enabled %}
{%     for file in  ['/etc/default/push-client'] %}
pushclient-config-{{file}}-billing:
    file.accumulated:
        - name: pushclient-default-config
        - filename: {{ file }}
        - text: /etc/pushclient/billing.conf
        - require_in:
            - file: {{ file }}
{%     endfor %}
{% endif %}

{% if shipping_enabled %}
/etc/pushclient/billing.conf:
    file.managed:
        - source: salt://components/dbaas-billing/conf/push-client-billing.conf
        - template: jinja
        - makedirs: True
        - require:
            - file: {{ log_dir }}
            - pkg: pushclient
        - watch_in:
            - service: pushclient

/etc/pushclient/billing.secret:
    file.managed:
{% if salt['pillar.get']('data:billing:tvm_client_id') %}
        - contents_pillar: 'data:billing:tvm_secret'
{% else %}
        - contents_pillar: 'data:billing:oauth_token'
{% endif %}
        - user: statbox
        - group: statbox
        - mode: 600
        - require:
            - file: /etc/pushclient/billing.conf
        - watch_in:
            - service: pushclient

/var/lib/push-client-billing:
    file.directory:
        - user: statbox
        - group: statbox
        - makedirs: True
        - require:
            - user: statbox-user
            - pkg: pushclient
        - require_in:
            - file: /etc/pushclient/billing.conf
{% else %}
/etc/pushclient/billing.conf:
    file.absent
{% endif %}


{% if use_cloud_logbroker and shipping_enabled %}
/etc/pushclient/billing-yc.conf:
    file.managed:
        - source: salt://components/dbaas-billing/conf/push-client-billing-yc.conf
        - template: jinja
        - makedirs: True
        - require:
            - file: {{ log_dir }}
            - pkg: pushclient
        - watch_in:
            - service: pushclient

/etc/pushclient/billing-yc.secret:
    file.managed:
        - contents: |
            {{ salt['pillar.get']('data:billing:yc_logbroker:iam:service_account') | tojson }}
        - user: statbox
        - group: statbox
        - mode: 600
        - require:
            - file: /etc/pushclient/billing-yc.conf
        - watch_in:
            - service: pushclient

/var/lib/push-client-billing-yc:
    file.directory:
        - user: statbox
        - group: statbox
        - makedirs: True
        - require:
            - user: statbox-user
            - pkg: pushclient
        - require_in:
            - file: /etc/pushclient/billing-yc.conf
{% else %}
# /etc/pushclient/billing-yc.conf:
#    file.absent
#
# /etc/pushclient/billing-yc.secret:
#    file.absent
{% endif %}
