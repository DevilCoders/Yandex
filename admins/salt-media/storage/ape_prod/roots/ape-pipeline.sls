{% set cgroup = grains['conductor']['group'] %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% set pgenv = 'production' %}
{% else %}
{% set pgenv = 'testing' %}
{% endif %}

include:
  - units.cocaine

/etc/cocaine/.cocaine/tools.yml:
  file.managed:
    - contents_pillar: yavape:cocaine.tools.yml
    - user: cocaine
    - mode: 444
    - makedirs: True

/home/cocaine/.pgpass:
  file.managed:
    - contents_pillar: yavape:cocaine.pgpass.{{ pgenv }}
    - user: cocaine
g   - mode: 600

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://ape-pipeline/etc/nginx/nginx.conf

/etc/cocaine-pipeline/cocaine-pipeline.conf:
  file.managed:
    - source: salt://ape-pipeline/etc/cocaine-pipeline/cocaine-pipeline.conf
    - makedirs: True

/etc/nginx/conf.d/01-accesslog.conf:
  file.managed:
    - source: salt://ape-pipeline/etc/nginx/conf.d/01-accesslog.conf

/home/cocaine/.docker/config.json:
  file.managed:
    - source: salt://docker/config.json

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://ape-pipeline/etc/nginx/sites-enabled/

/etc/apt/sources.list.d:
  file.recurse:
    - source: salt://ape-pipeline/etc/apt/source.list.d/

/etc/cocaine-v12-pipeline3-rest/cocaine-pipeline-rest.conf:
  file.managed:
    - source: salt://pipeline/etc/cocaine-v12-pipeline3-rest/cocaine-pipeline-rest.conf

/etc/nginx/ssl/pipeline.ape.yandex-team.ru.pem:
  file.managed:
    - source: "salt://certs/intape-front/pipeline.ape.yandex-team.ru.pem"
    - makedirs: True
    - mode: 600
    - user: root
    - group: root


