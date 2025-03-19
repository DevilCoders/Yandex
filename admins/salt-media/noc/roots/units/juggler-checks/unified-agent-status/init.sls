/etc/monrun/salt_unified-agent-status/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - yandex-media-common-oom-check

/etc/monrun/salt_unified-agent-status/unified_agent_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/unified_agent_status.sh
    - makedirs: True
    - mode: 755
