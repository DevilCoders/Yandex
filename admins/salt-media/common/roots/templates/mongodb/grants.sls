{% from slspath + '/map.jinja' import mongodb with context %}

grants-file:
{% if mongodb.get("grants-pillar", False) %}
  file.managed:
    - contents: {{ salt['pillar.get'](mongodb["grants-pillar"]) | json }}
{% else %}
  yafile.managed:
    - source:
      {% for source in mongodb.get("grants-sources", []) %}
      - salt://{{ source }}
      {% endfor %}
      - salt://mongo/files/mongo_grants_config
      - salt://mongo/grants/files/mongo_grants_config
{% endif %}
    - name: /var/cache/mongo-grants/mongo_grants_config
    - user: root
    - group: root
    - mode: 0600
    - makedirs: true

/usr/bin/check-mongo-grants:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/check-mongo-grants
    - user: root
    - group: root
    - mode: 0766

grant_roles:
  cmd.run:
    {% if mongodb.get("grants-check-only", False) %}
    {# only if explicitly enabled check only #}
    - name: /usr/bin/check-mongo-grants -c
    {% else %}
    {# by default check and apply !!BEWARE!! #}
    - name: /usr/bin/check-mongo-grants -g
    {% endif %}
    - require:
        {% if mongodb.get("grants-pillar", False) %}
        - file: grants-file
        {% else %}
        - yafile: grants-file
        {% endif %}
        - file: /usr/bin/check-mongo-grants
    - onchanges:
        {% if mongodb.get("grants-pillar", False) %}
        - file: grants-file
        {% else %}
        - yafile: grants-file
        {% endif %}
