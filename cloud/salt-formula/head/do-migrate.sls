prepare-migrate:
  cmd.script:
    - source: salt://{{ slspath }}/files/do-migrate.sh
    - env:
      - LC_ALL: C.UTF-8
      - LANG: C.UTF-8
