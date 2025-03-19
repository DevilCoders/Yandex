base packages:
  pkg.installed:
  - pkgs:
    - bsdmainutils
    - tcpdump
    - vim
    - strace
    - wget
    - zip
    - jq
    - lsof
    - unzip
    - curl
    - git
    - bind9-host
    - ethtool
    - ndisc6
    - traceroute
    - systemd-coredump

# https://github.com/GoogleCloudPlatform/compute-image-packages/blob/master/packages/python-google-compute-engine/README.md#configuration
# disable all except accounts automation
# https://st.yandex-team.ru/CLOUD-63656
pre-install config:
  file.managed:
    - name: /etc/default/instance_configs.cfg.template
    - source: salt://{{ slspath }}/instance_configs.cfg.template
    - mode: 0444

oslogin packages:
  pkg.installed:
  - pkgs:
    - python-google-compute-engine: 20190801-0ubuntu1~16.04.2
    - python3-google-compute-engine: 20190801-0ubuntu1~16.04.2
    - google-compute-engine-oslogin: 20200925.00-0ubuntu3~16.04.0
    - gce-compute-image-packages: 20190801-0ubuntu1~16.04.2
    require:
      - file: pre-install config
      - file: new template for cloud-init to form /etc/hosts

remove sudoers.d google_sudoers file:
  file.absent:
    - name: /etc/sudoers.d/google_sudoers

remove ipv4 only (https://github.com/GoogleCloudPlatform/compute-image-packages/pull/528):
  file.absent:
    - name: /etc/apt/apt.conf.d/99ipv4-only
