{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}


{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{{mongodb.config.get(srv).storage.dbPath}}:
    file.directory:
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 755
        - makedirs: True
{% endfor %}

{{mongodb.config_prefix}}:
    file.directory:
        - user: root
        - group: root
        - mode: 755
        - makedirs: True

{{mongodb.homedir}}:
    file.directory:
        - user: {{mongodb.user}}
        - group: {{mongodb.user}}
        - mode: 700
    require:
        - pkg: mongodb-packages

/var/log/mongodb:
    file.directory:
        - user: root
        - group: {{mongodb.user}}
        - mode: 775
