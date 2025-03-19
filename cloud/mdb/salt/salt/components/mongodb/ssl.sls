{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

{{mongodb.config_prefix}}/ssl:
    file.directory:
        - user: root
        - group: {{mongodb.user}}
        - makedirs: True
        - mode: 755
        - require_in:
{% for srv in mongodb.services_deployed %}
            - service: {{srv}}-service
{% endfor %}

{{mongodb.config_prefix}}/ssl/server.key:
    file.managed:
        - contents_pillar: cert.key
        - template: jinja
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 600
        - require:
            - file: {{mongodb.config_prefix}}/ssl

{{mongodb.config_prefix}}/ssl/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - template: jinja
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 600
        - require:
            - file: {{mongodb.config_prefix}}/ssl

{{mongodb.config_prefix}}/ssl/allCAs.pem:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 755
        - require:
            - file: {{mongodb.config_prefix}}/ssl
        - require_in:
{% for srv in mongodb.services_deployed %}
            - service: {{srv}}-service
{% endfor %}

{{mongodb.config_prefix}}/ssl/certkey.pem:  # https://docs.mongodb.com/v3.2/reference/configuration-options/#net.ssl.PEMKeyFile
    cmd.run:
      - shell: /bin/bash
      - require:
          - file: {{mongodb.config_prefix}}/ssl/server.key
          - file: {{mongodb.config_prefix}}/ssl/server.crt
      - require_in:
{% for srv in mongodb.services_deployed %}
          - service: {{srv}}-service
{% endfor %}
      - unless:
          - stat {{mongodb.config_prefix}}/ssl/certkey.pem
          - 'diff <(cat {{mongodb.config_prefix}}/ssl/server.key {{mongodb.config_prefix}}/ssl/server.crt) <(cat {{mongodb.config_prefix}}/ssl/certkey.pem)'
      - watch:
          - file: {{mongodb.config_prefix}}/ssl/server.key
          - file: {{mongodb.config_prefix}}/ssl/server.crt
      - name: |
          cat {{mongodb.config_prefix}}/ssl/server.key \
              {{mongodb.config_prefix}}/ssl/server.crt > {{mongodb.config_prefix}}/ssl/certkey.pem

{{mongodb.config_prefix}}/ssl/certkey.pem_grants:
    cmd.wait:
        - name: 'chown mongodb:mongodb {{mongodb.config_prefix}}/ssl/certkey.pem && chmod 0600 {{mongodb.config_prefix}}/ssl/certkey.pem'
        - watch:
            - cmd: {{mongodb.config_prefix}}/ssl/certkey.pem
