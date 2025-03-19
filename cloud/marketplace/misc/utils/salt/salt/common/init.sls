/etc/systemd/journald.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0600
    - source: salt://common/files/journald.conf


apt_helper_packages:
  pkg.installed:
    - pkgs:
      - python-apt

{%- for branch in ['stable', 'unstable'] %}
dist-yandex-cloud-all-{{ branch }}:
  pkgrepo.managed:
     - name: deb http://yandex-cloud.dist.yandex.ru/yandex-cloud {{ branch }}/all/
     - file: /etc/apt/sources.list.d/yandex-cloud-{{ branch }}.list

dist-yandex-cloud-amd64-{{ branch }}:
  pkgrepo.managed:
     - name: deb http://yandex-cloud.dist.yandex.ru/yandex-cloud {{ branch }}/amd64/
     - file: /etc/apt/sources.list.d/yandex-cloud-{{ branch }}.list
{%- endfor %}

oslogin packages:
  pkg.installed:
  - pkgs:
    - python-google-compute-engine
    - python3-google-compute-engine
    - google-compute-engine-oslogin
    - gce-compute-image-packages

ipv6 ssh only:
  file.uncomment:
    - name: /etc/ssh/sshd_config
    - regex: "ListenAddress ::"
