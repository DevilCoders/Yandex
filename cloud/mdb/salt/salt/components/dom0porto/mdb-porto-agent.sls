mdb-porto-agent-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-porto-agent: '1.9324668'
        - require:
            - file: /etc/mdb_porto_agent.yaml

/etc/mdb_porto_agent.cfg:
    file.absent

/etc/mdb_porto_agent.yaml:
    file.managed:
        - contents: |
            dbm_url: {{ salt['pillar.get']('data:config:dbm_url') }}
        - mode: 644
