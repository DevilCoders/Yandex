/etc/yc/safe-restart/safe-restart.yaml:
  file.managed:
    - makedirs: true
    - source: salt://{{ slspath }}/files/safe-restart.yaml
    - template: jinja
    - mode: 644

{# Renaming dns-named-restart.sh => restart-dns-named.sh.
   Remove this state if no "/etc/yc/safe-restart/dns-named-restart.sh" exist on oct-heads any more. #}
/etc/yc/safe-restart/dns-named-restart.sh:
  file.absent

/etc/yc/safe-restart/restart-dns-named.sh:
  file.managed:
    - makedirs: true
    - source: salt://{{ slspath }}/files/restart-dns-named.sh
    - mode: 755

/etc/yc/safe-restart/restart-vrouter-agent.sh:
  file.managed:
    - makedirs: true
    - source: salt://{{ slspath }}/files/restart-vrouter-agent.sh
    - mode: 755

/etc/yc/safe-restart/restart-vrouter-agent-and-kernel-module.sh:
  file.managed:
    - makedirs: true
    - source: salt://{{ slspath }}/files/restart-vrouter-agent-and-kernel-module.sh
    - mode: 755

/usr/local/bin/safe-restart:
  file.managed:
    - source: salt://{{ slspath }}/files/safe-restart.py
    - mode: 755
    - require:
        - file: /etc/yc/safe-restart/safe-restart.yaml
        - file: /etc/yc/safe-restart/restart-dns-named.sh
        - file: /etc/yc/safe-restart/restart-vrouter-agent.sh
        - file: /etc/yc/safe-restart/restart-vrouter-agent-and-kernel-module.sh
