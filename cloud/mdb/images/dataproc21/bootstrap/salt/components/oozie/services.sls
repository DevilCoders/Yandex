oozie-service:
    service.running:
        - enable: false
        - name: oozie
        - require:
            - pkg: oozie_packages
            - cmd: oozie-sharelib-init
            - cmd: oozie-database-exists
            - archive: oozie-extjs-extracted
            - hadoop-property: /etc/oozie/conf/oozie-site.xml
            - file: patch-/etc/oozie/conf/oozie-env.sh

oozie-available:
    dataproc.oozie_available:
        - require:
            - service: oozie-service