{% set oscodename = grains['oscodename'] %}

include:
  - templates.golang
  - templates.nodejs
  - templates.docker-ce
  - templates.ffmpeg
  - .scripts

/usr/local/bin/ott-wget:
  file.managed:
    - source: http://s3.mds.yandex.net/infrastructure/ott-wget
    - source_hash: ed3a2786b42347c1be374c0aafe159e2
    - makedirs: True
    - mode: 755

ffmpeg_common_packages:
  pkg.installed:
    - pkgs:
      - unrar
      - p7zip-full
      - gpac
      - libglu1-mesa
      {% if oscodename == 'trusty' %}
      - libgpac2
      {% else %}
      - libgpac4
      {% endif %}
      - gpac-modules-base
      - aria2
      - axel
      - audacity
      - awscli
      - python3-virtualenv
      - python3-pip
