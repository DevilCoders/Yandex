tvmtool-pkgs:
    pkg.installed:
        - pkgs:
            - yandex-passport-tvmtool: 1.3.4

{% if salt['pillar.get']('data:tvmtool:config:clients') %}

{% if salt['pillar.get']('data:tvmtool:config:secret') %}
# Check that secret and tvm_id don't specified 'globaly'
client-secret-absent-in-pillar:
    test.fail_without_changes:
        - name: 'specify secret for each data:tvmtool:config:clients conf'
        - require_in:
            - test: tvmtool-pillar
{% endif %}
{% if salt['pillar.get']('data:tvmtool:tvm_id') %}
tmv-id-absent-in-pillar:
    test.fail_without_changes:
        - name: 'specify tvm_id for each data:tvmtool:config:clients:* conf'
        - require_in:
            - test: tvmtool-pillar
{% endif %}
tvmtool-pillar:
    test.check_pillar:
        - present:
            - data:tvmtool:token
        - integer:
            - data:tvmtool:port

{% else %}

tvmtool-pillar:
    test.check_pillar:
        - present:
            - data:tvmtool:token
            - data:tvmtool:config:secret
        - integer:
            - data:tvmtool:port
            - data:tvmtool:tvm_id

{% endif %}

/etc/tvmtool/tvmtool.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/tvmtool.conf' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: tvmtool-pkgs
            - test: tvmtool-pillar

/var/lib/tvmtool/local.auth:
    file.managed:
        - contents_pillar: data:tvmtool:token
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: tvmtool-pkgs
            - test: tvmtool-pillar

{% if salt['pillar.get']('data:tvmtool:workarounds:maps-127.0.0.1-to-localhost') %}
# in compute we don't manage /etc/hosts
# Problem:
#  * TVM Daemon (aka tvmtool) binds to 'localhost'
#  * Some TVM-Clients (go LogBroker SDK - persqueue) try communicate with TVM Daemon on 127.0.0.1
maps-127.0.0.1-to-localhost:
    host.present:
        - ip: '127.0.0.1'
        - names:
            - localhost
        - require_in:
            - service: tvmtool-service
{% endif %}

tvmtool-service:
    service.running:
        - name: yandex-passport-tvmtool
        - enable: True
        - watch:
            - pkg: tvmtool-pkgs
            - file: /var/lib/tvmtool/local.auth
            - file: /etc/tvmtool/tvmtool.conf


