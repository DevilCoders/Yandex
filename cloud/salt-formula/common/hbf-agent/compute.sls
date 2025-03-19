include:
  - common.hbf-agent

/etc/default/hbf-agent:
  file.managed:
    - template: jinja
    - makedirs: True
    - mode: 0644
    - source: salt://{{ slspath }}/hbf-agent-compute
    - watch_in:
      - service: hbf-agent-pkg
    - require_in:
      - service: hbf-agent-pkg

/usr/lib/hbf-agent/collect_ips.d/01-contrail-ports:
  file.managed:
    - makedirs: True
    - mode: 0755
    - source: salt://{{ slspath }}/collect-vm-ips.py
    - require:
      - yc_pkg: hbf-agent-pkg
    - watch_in:
      - service: hbf-agent-pkg

/etc/sudoers.d/hbf_sudoers:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/hbf_sudoers
    - require:
      - yc_pkg: hbf-agent-pkg
      - yc_pkg: yc-compute-node
