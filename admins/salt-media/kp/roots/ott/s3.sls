s3-packages:
  pkg.installed:
    - pkgs:
      - yandex-media-s3cmd

#{% for user in grains['cauth']['users'] %}
#/home/{{ user }}/.s3cfg:
#  file.managed:
#    - source: salt://s3-secure/s3cfg
#    - onlyif: "[ -d /home/{{ user }} ]"
#{% endfor %}

/root/.s3cfg:
  file.managed:
    - contents: {{ salt['pillar.get']('s3-secure:s3cfg') | json }}

