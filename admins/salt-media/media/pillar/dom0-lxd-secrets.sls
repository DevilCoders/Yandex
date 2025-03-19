{% set ctags = salt['grains.get']('conductor:tags') %}
{% set secrets = salt.yav.get('sec-01czwnxbqqfsevkn11j4wnvjgg') %}
lxd:
  projects:
    - name: bootstrap
      config:
        environment.LXD_CONTAINER_DOMAIN: .yandex.net
        environment.LXD_CONDUCTOR_TOKEN: {{ secrets.bootstrap_conductor_token | json }}
    - name: media
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: media-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x670"
      {% endif %}
      {% if 'media-dom0-music' in salt['grains.get']('c') %}
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
      {% else %}
      devices:
        eth0:
          type: nic
          {% if 'PID' in ctags %}
          nictype: macvlan
          parent: eth0
          {% else %}
          nictype: bridged
          parent: br0
          {% endif %}
      {% endif %}
    - name: media-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: media-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x423d"
      {% endif %}
      {% if 'media-dom0-music' in salt['grains.get']('c') %}
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
      {% else %}
      devices:
        eth0:
          type: nic
          {% if 'PID' in ctags %}
          nictype: macvlan
          parent: eth0
          {% else %}
          nictype: bridged
          parent: br0.639
          {% endif %}
      {% endif %}
    - name: music
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.music_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: music-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
        environment.LXD_HBF: 1
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
        user.prjid: "0x671"
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
    - name: music-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.music_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: music-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
        environment.LXD_HBF: 1
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
        user.prjid: "0x441f"
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
    - name: music-dev
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.music_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: music-lost
        environment.LXD_YANDEX_ENVIRONMENT: development
        environment.LXD_HBF: 1
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
        user.prjid: "0x4311"
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
    - name: music-qa
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.music_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: music-lost
        environment.LXD_YANDEX_ENVIRONMENT: qa
        environment.LXD_HBF: 1
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
        user.prjid: "0x4421"
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
    - name: mediabilling
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: mediabilling-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
        environment.LXD_HBF: 1
        user.prjid: "0x670"
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
    - name: mediabilling-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: mediabilling-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
        environment.LXD_HBF: 1
        user.prjid: "0x639"
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
    - name: mediabilling-dev
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: mediabilling-lost
        environment.LXD_YANDEX_ENVIRONMENT: development
        environment.LXD_HBF: 1
        environment.LXD_MTN_RA_DISABLE: 1
        environment.LXD_SKIP_MANAGE_NETWORK: 1
        user.prjid: "0x639"
      devices:
        eth0:
          type: nic
          mtu: "9000"
          nictype: p2p
    - name: afisha
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.afisha_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: afisha-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x1329"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.1329
          type: nic
      {% endif %}
    - name: afisha-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.afisha_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: afisha-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x1451"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: afisha-stress
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.afisha_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: afisha-lost
        environment.LXD_YANDEX_ENVIRONMENT: stress
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x568"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.568
          type: nic
      {% endif %}
    - name: kassa
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.kassa_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: kassa-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x1357"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.1357
          type: nic
      {% endif %}
    - name: kassa-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.kassa_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: kassa-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x1451"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: kassa-stress
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.kassa_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: kassa-lost
        environment.LXD_YANDEX_ENVIRONMENT: stress
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x568"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.568
          type: nic
      {% endif %}
    - name: jkp
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.jkp_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: jkp_lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x1556"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.1556
          type: nic
      {% endif %}
    - name: jkp-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.jkp_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: jkp_lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: jkp-stress
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.jkp_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: jkp_lost
        environment.LXD_YANDEX_ENVIRONMENT: stress
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: kp
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.jkp_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: kp-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x556"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.556
          type: nic
      {% endif %}
    - name: kp-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.jkp_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: kp-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: kp-stress
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.jkp_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: kp-lost
        environment.LXD_YANDEX_ENVIRONMENT: stress
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: tv-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.content_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: content_lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: tv
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.content_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: content_lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x507"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.507
          type: nic
      {% endif %}
    - name: kino-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.content_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: content_lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: kino
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.content_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: content_lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x507"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.507
          type: nic
      {% endif %}
    - name: kino-stress
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.content_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: content_lost
        environment.LXD_YANDEX_ENVIRONMENT: stress
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x568"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.568
          type: nic
      {% endif %}
    - name: tv-stress
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.content_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: content_lost
        environment.LXD_YANDEX_ENVIRONMENT: stress
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x568"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.568
          type: nic
      {% endif %}
    - name: cult
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.cult_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: cult-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x623"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.623
          type: nic
      {% endif %}
    - name: ott
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: ott-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x670"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0
          type: nic
      {% endif %}
    - name: ott-test
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: ott-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: ott-dev
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: ott-lost
        environment.LXD_YANDEX_ENVIRONMENT: development
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.639
          type: nic
      {% endif %}
    - name: ott-stress
      config:
        environment.LXD_DNS_TOKEN: {{ secrets.media_dns_token | json }}
        environment.LXD_CONDUCTOR_GROUP: ott-lost
        environment.LXD_YANDEX_ENVIRONMENT: stress
      {% if 'PID' in ctags %}
        environment.LXD_HBF: 1
        user.prjid: "0x639"
      devices:
        eth0:
          nictype: macvlan
          parent: eth0
          type: nic
      {% else %}
      devices:
        eth0:
          nictype: bridged
          parent: br0.568
          type: nic
      {% endif %}

{% if 'dom0_music' in ctags %}
  {% set pool = 'music' %}
{% elif 'dom0_alet' in ctags %}
  {% set pool = 'alet' %}
{% elif 'dom0_tokk' in ctags %}
  {% set pool = 'tokk' %}
{% else %}
  {% set pool = 'media' %}
{% endif %}

icecream:
  agent:
    enabled: True
    config:
      pool: {{ pool }}
      sentry: {{ secrets.icecream_sentry | json }}
      api:
        host: https://ice.yandex-team.ru
        token: {{ secrets.icecream_api | json }}
      storage:
        name: lxd
        type: lvm

selfdns:
  token: {{ secrets.media_dns_token | json }}
