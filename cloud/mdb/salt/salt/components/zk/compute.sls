{% set zk = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

zookeeper-java-created:
    cmd.run:
        - name: ln /usr/lib/jvm/java-11-openjdk-{{ salt['grains.get']('osarch') }}/bin/java {{zk.java_bin}}
        - require:
            - pkg: zookeeper-packages
            - pkg: java-packages
        - require_in:
            - service: zookeeper-service
        - unless: test -e {{zk.java_bin}}

# Need a specific jre version because of the trickery above
java-packages:
    pkg.installed:
        - name: openjdk-11-jdk-headless
