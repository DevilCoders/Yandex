{% from slspath ~ "/pg.jinja" import pg with context %}
{% set osrelease = salt['grains.get']('osrelease') %}
{{pg.prefix}}/.gnupg:
    file.directory:
        - user: postgres
        - group: postgres
        - makedirs: True
        - mode: 700

{% for file in ['pubring.gpg', 'secring.gpg', 'trustdb.gpg'] %}
{{pg.prefix}}/.gnupg/{{file}}.base64:
    file.managed:
        - contents_pillar: {{file}}
        - user: postgres
        - group: postgres
        - mode: 600
        - require:
            - file: {{pg.prefix}}/.gnupg
install-{{file|replace('.', '-')}}:
    cmd.wait:
        - shell: /bin/bash
        - runas: postgres
        - group: postgres
        - umask: 077
        - require:
            - file: {{pg.prefix}}/.gnupg/{{file}}.base64
        - require_in:
{% if salt['pillar.get']('data:use_walg', True) %}
            - file: /etc/wal-g/wal-g.yaml
{% endif %}
        - watch:
            - file: {{pg.prefix}}/.gnupg/{{file}}.base64
        - name: |
            openssl enc -base64 -d -in {{pg.prefix}}/.gnupg/{{file}}.base64 -out {{pg.prefix}}/.gnupg/{{file}}
{% endfor %}
