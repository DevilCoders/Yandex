{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

encryption_req:
  test.nop:
    - require:
      - file: {{mongodb.config_prefix}}/ssl
    - require:
      - file: {{mongodb.config_prefix}}/ssl/kmip_ca.pem
      - file: {{mongodb.config_prefix}}/ssl/kmip_client.pem

encryption_done:
  test.nop:
    - require_in:
{% for srv in mongodb.services_deployed %}
      - service: {{srv}}-service
{% endfor %}
    - require:
      - file: {{mongodb.config_prefix}}/ssl/kmip_ca.pem
      - file: {{mongodb.config_prefix}}/ssl/kmip_client.pem

{% if mongodb.config.mongod.security.enableEncryption %}

{{mongodb.config_prefix}}/ssl/kmip_ca.pem:
    file.managed:
        - contents_pillar: data:mongodb:config:mongod:security:kmip:serverCa
        - template: jinja
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 600

{{mongodb.config_prefix}}/ssl/kmip_client.pem:
    file.managed:
        - contents_pillar: data:mongodb:config:mongod:security:kmip:clientCertificate
        - template: jinja
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 600

{% else %}

{{mongodb.config_prefix}}/ssl/kmip_ca.pem:
    file.absent

{{mongodb.config_prefix}}/ssl/kmip_client.pem:
    file.absent

{% endif %}
