kdump:
  yc_pkg.installed:
    - pkgs:
      - linux-crashdump
      - kdump-tools
    # linux-crashdump recommends apport which should not be installed
    # (apport conflicts with contrail-vrouter-agent)
    - install_recommends: False

{% if "crashkernel" in grains['cmdline'] %}
/etc/default/kdump-tools:
  file.managed:
    - source: salt://{{ slspath }}/files/kdump-tools
    - template: jinja
    - require:
      - yc_pkg: kdump

reload_kdump_config:
  cmd.run:
    - names:
      - kdump-config unload && kdump-config load
    - require:
      - file: /etc/default/kdump-tools
    - onchanges:
      - file: /etc/default/kdump-tools
{% endif %}
