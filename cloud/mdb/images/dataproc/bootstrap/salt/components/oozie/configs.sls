{% set oozie_config_path = salt['pillar.get']('data:config:oozie_config_path', '/etc/oozie/conf') %}
{% set oozie_path = '/usr/lib/oozie' %}

{% import 'components/oozie/oozie-site.sls' as c with context %}

{{ oozie_config_path }}/oozie-site.xml:
    hadoop-property.present:
        - config_path: {{ oozie_config_path }}/oozie-site.xml
        - properties:
            {{ c.config_site['oozie'] | json }}
        - require:
            - pkg: oozie_packages

patch-{{ oozie_config_path }}/oozie-env.sh:
    file.append:
        - name: {{ oozie_config_path }}/oozie-env.sh
        - text:
            - ' '
            - '# Forcing IPv4 setting for jetty server'
            - 'export JETTY_OPTS="${JETTY_OPTS} -Djava.net.preferIPv6Stack=false -Djava.net.preferIPv4Stack=true" '

{{ oozie_path }}/bin/install_oozie.sh:
    file.managed:
        - name: {{ oozie_path }}/bin/install_oozie.sh
        - source: salt://{{ slspath }}/bin/install_oozie.sh
        - mode: 755
        - require:
            - pkg: oozie_packages
