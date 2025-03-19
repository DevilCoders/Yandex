
/etc/apt/trusted.gpg.d/deadsnakes-gpg.key:
  file.managed:
    - source: salt://{{ slspath }}/files/deadsnakes.apt.key
  cmd.run:
    - name: apt-key add /etc/apt/trusted.gpg.d/deadsnakes-gpg.key
    - onchanges:
      - file: /etc/apt/trusted.gpg.d/deadsnakes-gpg.key

deadsnakes-ppa:
  pkgrepo.managed:
    - name: deb https://mirror.yandex.ru/mirrors/launchpad/deadsnakes/ppa bionic main
    - dist: bionic
    - file: /etc/apt/sources.list.d/deadsnakes-bionic.list
    - refresh_db: true

python3.7-full:
  pkg.installed:
    - require:
      - pkgrepo: deadsnakes-ppa

