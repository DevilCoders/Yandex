firewall-packages:
    pkg.installed:
        - pkgs:
{% for package, version in salt['pillar.get']('firewall:packages', {'ferm': 'any'}).items() %}
{% if version == 'any' %}
            - {{package}}
{% else %}
            - {{package}}: {{version}}
{% endif %}
{% endfor %}
        - require_in:
            - test: ferm-ready
