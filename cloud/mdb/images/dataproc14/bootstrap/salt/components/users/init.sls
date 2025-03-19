{% if salt['pillar.get']('data:ssh_public_keys', [])|length > 0 -%}
ssh-root-keys:
    ssh_auth.present:
        - user: root
        - names:
        {% for key in salt['pillar.get']('data:ssh_public_keys') %}
            - {{ key }}
        {% endfor %}
{%- endif %}
