real hbf agent:
  pkg.installed:
    - pkgs:
      - yandex-hbf-agent-init
      - yandex-hbf-agent-static

yandex-hbf-agent:
  service.running:
    - enable: True
    - reload: True