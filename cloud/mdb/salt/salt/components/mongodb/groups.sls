{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
extend:
    cores-group:
        group:
            - members:
                - {{mongodb.user}}
