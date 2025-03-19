{% set framework_path='/usr/bin/yc-marketplace' %}
{% set build_secret_config_path='/etc/yc-marketplace/build_service_account.json' %}

/framework.tar.gz:
  file.managed:
  - source: salt://{{sls}}/files/framework.tar.gz
  - mode: 0666

'Extract Framework archive':
  cmd.run:
  - name: mkdir -p {{ framework_path }}/framework && tar -zxvf /framework.tar.gz -C {{ framework_path }}/framework

/etc/yc-marketplace/fabrica-config.yaml:
  file.managed:
  - source: salt://{{sls}}/files/config.yaml
  - user: root
  - group: root
  - mode: 0644
  - makedirs: True
  - template: jinja
  - defaults:
      framework_path: {{framework_path}}

packages:
  pkg.installed:
  - pkgs:
    - yandex-config-dns64
    - parted
    - qemu-system
    - qemu-utils
    - s3cmd
    - yandex-internal-root-ca
    - jq
    - apt-transport-https
    - systemd-container


{{ build_secret_config_path }}:
  file.managed:
    - source: salt://{{sls}}/files/build_service_account.json.jinja
    - user: root
    - group: root
    - mode: 0600
    - makedirs: True
    - template: jinja

'Install packer':
  cmd.run:
  - name: wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 3 https://storage.yandexcloud.net/yc-marketplace-distr/packer_1.6.1_linux_amd64.zip -O /tmp/packer.zip && sudo unzip -o /tmp/packer.zip -d /usr/local/bin

'Export YC_ENDPOINT':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_ENDPOINT=.*$' | regex_escape }}
    - repl: YC_ENDPOINT={{ pillar['factory']['cloud_public_api'] }}
    - append_if_not_found: True

'Export YC_BUILD_SERVICE_ACCOUNT_SECRET':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_BUILD_SERVICE_ACCOUNT_SECRET=.*$' | regex_escape }}
    - repl: YC_BUILD_SERVICE_ACCOUNT_SECRET={{ build_secret_config_path }}
    - append_if_not_found: True

'Export YC_BUILD_SUBNET':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_BUILD_SUBNET=.*$' | regex_escape }}
    - repl: YC_BUILD_SUBNET={{ grains['YC_BUILD_SUBNET'] }}
    - append_if_not_found: True

'Export YC_ZONE':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_ZONE=.*$' | regex_escape }}
    - repl: YC_ZONE={{ grains['YC_ZONE'] }}
    - append_if_not_found: True

'Export YC_BUILD_FOLDER_ID':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_BUILD_FOLDER_ID=.*$' | regex_escape }}
    - repl: YC_BUILD_FOLDER_ID={{ pillar['factory']['build_folder_id'] }}
    - append_if_not_found: True

'Export YC_DIRTY_IMAGES_FOLDER_ID':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_DIRTY_IMAGES_FOLDER_ID=.*$' | regex_escape }}
    - repl: YC_DIRTY_IMAGES_FOLDER_ID={{ pillar['factory']['dirty_images_folder_id'] }}
    - append_if_not_found: True

'Export YC_MKT_DISTR_S3_SECRET_KEY':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_MKT_DISTR_S3_SECRET_KEY=.*$' | regex_escape }}
    - repl: YC_MKT_DISTR_S3_SECRET_KEY={{ pillar['security']['factory']['mkt_distr_s3']['secret_key'] }}
    - append_if_not_found: True

'Export YC_MKT_DISTR_S3_ACCESS_KEY':
  file.replace:
    - name: /etc/environment
    - pattern: {{ '^YC_MKT_DISTR_S3_ACCESS_KEY=.*$' | regex_escape }}
    - repl: YC_MKT_DISTR_S3_ACCESS_KEY={{ pillar['security']['factory']['mkt_distr_s3']['access_key'] }}
    - append_if_not_found: True

#iptables -A OUTPUT -p tcp -m tcp --dport 22 -j ACCEPT # SSH
'Allow ssh output':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: OUTPUT
    - protocol: tcp
    - match: tcp
    - dport: 22
    - jump: ACCEPT
    - save: True

#iptables -A OUTPUT -p tcp -m tcp --dport 5986 -j ACCEPT # winrm
'Allow winrm output':
  iptables.append:
    - table: filter
    - family: ipv4
    - chain: OUTPUT
    - protocol: tcp
    - match: tcp
    - dport: 5986
    - jump: ACCEPT
    - save: True

