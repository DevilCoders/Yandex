
monrun-kafka-pkgs:
    pkg.installed:
        - pkgs:
            - python-pip
            - python3-pip

install-kafka-python2:
    pip.installed:
        - name: confluent-kafka==1.3.0
        - bin_env: /usr/bin/pip
        - require:
            - pkg: monrun-kafka-pkgs

install-kafka-python3:
    pip.installed:
        - pkgs:
            - setuptools
            - wheel
        - bin_env: /usr/bin/pip3
        - require:
            - pkg: monrun-kafka-pkgs
        - require_in:
            - file: monrun-kafka-scripts
    pkg.installed:
        - pkgs:
            - python36-confluent-kafka: 1.8.2

monrun-kafka-scripts:
    file.recurse:
        - name: /usr/local/yandex/monitoring
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-kafka-confs:
    file.recurse:
        - name: /etc/monrun/conf.d
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

monrun-kafka-sudoers:
    file.recurse:
        - name: /etc/sudoers.d
        - file_mode: '0640'
        - template: jinja
        - source: salt://{{ slspath }}/sudoers.d
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update

{% if salt['pillar.get']('data:kafka:has_zk_subcluster', false) %}
/etc/monrun/conf.d/zookeeper.conf:
    file.managed:
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf/zookeeper.conf
        - watch_in:
            - cmd: monrun-jobs-update
{% endif %}
