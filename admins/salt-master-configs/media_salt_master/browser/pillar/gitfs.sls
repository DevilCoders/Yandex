{% set senv = salt['grains.get']('yandex-environment', 'base')|replace("production", "stable") %}

salt_master:
  lookup:
    default: False
    ssh: {'key_path': '/repo/projects/corba/browser/salt', 'key_name': '{{ senv }}-salt.id_rsa'}
    file_roots:
      {{ senv }}:
        - /srv/salt/roots
    gitfs_remotes:
      - ssh://git@bitbucket.browser.yandex-team.ru/broadm/salt.git:
        - root: roots
        - name: browser
      - ssh://git@bitbucket.browser.yandex-team.ru/broadm/salt.git:
        - root: formulas
        - name: formulas
      - ssh://robot-media-salt@git.yandex.ru/media_salt.git:
        - root: common/roots
        - name: common
    ext_pillar:
      - git:
        - {{ senv }} git@bitbucket.browser.yandex-team.ru:broadm/salt.git:
          - root: pillar
