{% set knox_bin_path = '/usr/lib/knox/bin' %}

{% if salt['ydputils.is_masternode']() %}

knox-generate-master-secret:
    cmd.run:
        - name: {{ knox_bin_path }}/knoxcli.sh create-master --master dataproc
        - unless: test -f "/usr/lib/knox/data/security/master"
        - runas: knox
        - require:
            - pkg: knox_packages

{% endif %}
