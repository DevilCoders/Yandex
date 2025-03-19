usr_envs:
    users:
        - root
        {% for user in salt.grains.get('conductor:admins') %}
        - {{ user }}
        {% endfor %}
