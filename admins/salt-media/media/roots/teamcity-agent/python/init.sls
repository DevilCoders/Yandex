python_pkgs:
  pkg.installed:
    - pkgs:
      - build-essential
      - zlib1g-dev
      - libncurses5-dev
      - libgdbm-dev
      - libnss3-dev
      - libssl-dev
      - libreadline-dev
      - libffi-dev
      - libsqlite3-dev
      - wget
      - libbz2-dev

/usr/local/bin/python_install.sh:
  file.managed:
    - mode: 755
    - source: salt://{{slspath}}/python_install.sh

python_install:
  cmd.run:
    - name: /usr/local/bin/python_install.sh
    - unless: python3.9 --version

