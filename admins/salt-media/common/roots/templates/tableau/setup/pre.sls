tsmadmin:
  group.present:
    - name: tsmadmin

tableau_user:
  user.present:
    - name: {{ salt['pillar.get']('admin_user') }}

create-directories:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
    - names:
      - /usr/local/share/ca-certificates/Yandex
      - /tmp/tableau

install-yandex-root-ca:
   cmd.run:
     - name: |
          wget https://storage.yandexcloud.net/cloud-certs/CA.pem -O \
            /usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt && \
          chmod 655 /usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt

setup-tun-iface:
  cmd.run:
    - name: /usr/lib/yandex-netconfig/ya-slb-tun restart
    - env:
      - IF_YA_SLB_TUN: "YES"
      - IF_YA_SLB6_TUN: "YES"
      - DEBUG: "YES"

/etc/rc.conf.local:
   file.managed:
     - source: salt://{{ slspath }}/files/rc.conf.local
     - user: root
     - group: root
     - mode: 0755
