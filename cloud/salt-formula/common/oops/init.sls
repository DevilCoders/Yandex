yandex-oops-agent:
  yc_pkg.installed:
    - pkgs:
      - yandex-oops-agent

sudo_rules_for_yandex-oops-agent:
  file.managed:
    - name: /etc/sudoers.d/yandex-oops
    - source: salt://{{ slspath }}/files/yandex-oops
    - require:
      - yc_pkg: yandex-oops-agent
