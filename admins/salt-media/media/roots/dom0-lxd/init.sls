include:
  - templates.packages
  - templates.lxd
  - templates.selfdns
  - templates.nginx
  - templates.icecream.agent
  - .watchdog
  - .graphite-sender-script
  - .hbf-agent

set MSK timezone:
  cmd.run:
    - name: |
        #!/bin/bash
        echo 'Europe/Moscow' > /etc/timezone
        cp -f /usr/share/zoneinfo/Europe/Moscow /etc/localtime.dpkg-new
        mv -f /etc/localtime.dpkg-new /etc/localtime
        dpkg-reconfigure -f noninteractive tzdata
        service cron restart || true
    - unless: date | grep MSK

/etc/modules-load.d/03-tunnel.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        ip6_tunnel
        ip_tunnel

net.ipv6.conf.all.use_tempaddr:
  sysctl.present:
    - value: 0

net.ipv6.conf.default.use_tempaddr:
  sysctl.present:
    - value: 0

/etc/monitoring/la.conf:
  file.managed:
    - user: root
    - group: root
{% if grains['fqdn'] == 'man1-8996.media.yandex.net' %}
    - contents: |
        200
{% else %}
    - contents: |
        100
{% endif %}
