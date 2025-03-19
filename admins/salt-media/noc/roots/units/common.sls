include:
  - units.journald

{% if grains['os_family'] == "Debian" %}
bad_pkgs:
  pkg.purged:
    - pkgs:
      - yandex-media-common-hw-watcher-conf
      - netplan.io
      {% if grains["virtual"] == "lxc" %}
      - apparmor
      {% endif %}
      {% if pillar["remove_yandex_netconfig"]|default(true) %}
      - yandex-netconfig
      {% endif %}
{% endif %}

# Disable mail reports from smartd
# Using cmd.run instead of service.dead because of bug with test mode(section always marked as changed)
'sudo systemctl disable smartmontools && sudo systemctl stop smartmontools':
  cmd.run:
    - unless: '! systemctl is-active smartmontools'

