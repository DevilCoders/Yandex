{% set is_prod = grains['yandex-environment'] in ['production', 'prestable'] %}
{% set is_stress = grains['yandex-environment'] in ['stress'] %}
{% set dblist = {
    'couldlike':        'couldlike',
    'favouritefriends': 'ffriends',
    'kinopoisk':        'kinopoisk',
    'parser':           'parser',
    'shop':             'shop',
    'stars':            'stars',
    'translate':        'translate',
    'ur':               'ur',
    'yablog':           'yablog',
} %}

{% if is_prod %}
{% set hostname = salt['cmd.shell']("python3 -c \"import requests; [print(requests.get('http://c.yandex-team.ru/api-cached/groups2hosts/kp-db-master').text)]\" | xargs") %}
{% elif is_stress %}
{% set hostname = salt['cmd.shell']("python3 -c \"import requests; [print(requests.get('http://c.yandex-team.ru/api-cached/groups2hosts/kp-load-db-master').text)]\" | xargs") %}
{% else %}
{% set hostname = salt['cmd.shell']("python3 -c \"import requests; [print(requests.get('http://c.yandex-team.ru/api-cached/groups2hosts/kp-test-db-master').text)]\" | xargs") %}
{% endif %}


{% for db, login in dblist.items() %}
/usr/share/kinopoisk-source-migrations/auto/{{ db }}/flyway.properties:
    file.managed:
        - template: jinja
        - makedirs: True
        - contents: |
              {% if is_prod -%}
              flyway.url=jdbc:mysql://{{ hostname }}:3306/{{ db }}
              {% elif is_stress -%}
              flyway.url=jdbc:mysql://{{ hostname }}:3306/{{ db }}
              {% else -%}
              flyway.url=jdbc:mysql://{{ hostname }}:3306/{{ db }}
              {% endif -%}
              flyway.user={{ salt['pillar.get']('flyway:user') }}{{ login }}
              flyway.password={{ salt['pillar.get']('flyway:password') }}{{ login }}
              flyway.outOfOrder=true
{% endfor %}
