common-packages:
  pkg.installed:
    - refresh: False
    - pkgs:
      - bigtop-utils
      - bigtop-jsvc
      - google-compute-engine-oslogin

/root/.bashrc:
  file.append:
    - text: alias hs='salt-call --out=highstate --state-out=changes --output-diff --log-file-level=info --log-level=quiet state.highstate queue=True'

/home/ubuntu/.bashrc:
  file.append:
    - text:
      - alias hs='sudo salt-call --out=highstate --state-out=changes --output-diff --log-file-level=info --log-level=quiet state.highstate queue=True'
      # https://askubuntu.com/questions/22037/aliases-not-available-when-using-sudo
      - alias sudo='sudo '

/usr/local/share/ca-certificates/yandex-cloud-ca.crt:
  file.managed:
    - source:
      - https://storage.yandexcloud.net/cloud-certs/CA.pem
      - salt://{{ slspath }}/CA.pem
    - mode: 0644
    - skip_verify: True
  cmd.run:
    - name: 'update-ca-certificates --fresh'
    - onchanges:
      - file: /usr/local/share/ca-certificates/yandex-cloud-ca.crt

# temporary duplication to break dependency of cloud/mdb/dataproc-infra-tests/tests/helpers/pillar.py:232
# remove after new images will become stable
/srv/CA.pem:
  file.managed:
    - source:
      - /usr/local/share/ca-certificates/yandex-cloud-ca.crt
    - mode: 0644
    - skip_verify: True
    - require:
      - file: /usr/local/share/ca-certificates/yandex-cloud-ca.crt

/var/log/yandex:
  file.directory:
    - user: syslog
    - group: syslog
    - dir_mode: 755
    - file_mode: 644

oslogin-rsa-allowed:
  file.append:
    - name: /etc/ssh/sshd_config
    - text:
      - ' '
      - '# Yandex.Cloud: Temporary allow rsa keys for oslogin'
      - 'CASignatureAlgorithms ^ssh-rsa'

oslogin-enabled:
  cmd.run:
    - name: google_oslogin_control activate
    - unless: google_oslogin_control status
    - require:
      - pkg: common-packages
      - file: oslogin-rsa-allowed

{% set packages = salt['ydputils.get_additional_packages']() %}
{% if packages != [] %}
additional-package:
  pkg.installed:
    - refresh: False
    - retry:
      attempts: 5
      until: True
      interval: 60
      splay: 10
    - pkgs:
{% for package in packages %}
      - {{ package }}
{% endfor %}
    - require:
      - pkgrepo: ppa-repositories
    - require_in:
      service: dataproc-agent-service
{% endif %}
