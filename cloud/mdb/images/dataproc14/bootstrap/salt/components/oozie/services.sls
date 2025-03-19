oozie-service:
    service:
        - running
        - enable: true
        - name: oozie
        - parallel: true
        - require:
            - pkg: oozie_packages
            - cmd: oozie-sharelib-init
            - cmd: oozie-database-exists
            - archive: oozie-extjs-extracted
            - hadoop-property: /etc/oozie/conf/oozie-site.xml
            - file: patch-/etc/oozie/conf/oozie-env.sh
