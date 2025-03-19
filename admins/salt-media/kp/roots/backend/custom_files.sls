{% set is_prod = grains['yandex-environment'] in ['production', 'prestable'] %}
{% set is_test = grains['yandex-environment'] in ['testing'] %}

# only for production
{% if is_prod %} # Only for compatibility on production. Must be deleted in future
/var/www/.ssh/id_rsa:
  file.managed:
    - contents: {{salt.pillar.get('www_data:id_rsa')|json}}
    - user: www-data
    - group: www-data
    - mode: 600
    - makedirs: true

/usr/local/bin/s3-config-graphdata.yaml:
  file.managed:
    - contents: {{ pillar['s3config-graphdata_secret']|json }}
    - user: www-data
    - group: www-data
    - mode: 0440
    - makedirs: True
{% endif %}

{% if is_prod or is_test %}
/usr/local/bin/s3-config-static.yaml:
  file.managed:
    - contents: {{ pillar['s3config-static_secret']|json }}
    - user: www-data
    - group: www-data
    - mode: 0440
    - makedirs: True
{% endif %}

boto3:
  pkg.installed:
    - pkgs:
      - python3-boto3

api_scripts_pkgs:
  pkg.installed:
    - names:
      - python3-yaml
      - yandex-kinoposk-api-scripts
