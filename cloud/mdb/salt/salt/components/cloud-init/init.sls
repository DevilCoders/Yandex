{% if salt.pillar.get('data:running_on_template_machine', False) == True %}
cloud-init-pkg:
    pkg.installed:
        - pkgs:
            - cloud-init: '20.3-2-g371b392c-0ubuntu1~18.04.1'
            - patch: '2.7.6-2ubuntu1.1'
        - require:
            - cmd: repositories-ready

# this awaits https://github.com/canonical/cloud-init/pull/746 to be brought to deb package,
# then it should be removed
/usr/lib/python3/dist-packages/cloudinit/netinfo.py:
    file.patch:
        - source: salt://{{ slspath }}/patches/cloud_init_netinfo_ipv6.diff
        - hash: sha1=a8c71d1261e4116fbd4161e6ef11e2184830acbb
        - require:
              - pkg: cloud-init-pkg

/etc/cloud/cloud.cfg.d/01_yc_Ec2.cfg:
    file.managed:
        - source: salt://{{ slspath }}/conf/01_yc_Ec2.yaml
        - template: jinja
        - mode: 644
        - user: root
        - group: root
        - require:
              - pkg: cloud-init-pkg

/etc/cloud/cloud.cfg.d/95_mdb_ds.cfg:
    file.managed:
        - source: salt://{{ slspath }}/conf/95_mdb_ds.yaml
        - template: jinja
        - mode: 644
        - user: root
        - group: root
        - require:
              - pkg: cloud-init-pkg
{% else %}
cloud-init-pkg:
    pkg.purged:
        - name: cloud-init

/etc/cloud:
    file.absent:
        - require:
              - pkg: cloud-init-pkg
{% endif %}
