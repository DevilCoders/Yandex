repo_pkgs:
  pkg.installed:
    - pkgs:
      - yandex-conf-repo-php-ondrej
      - percona-repo-config
    - watch_in:
      cmd: 'apt-get -q update'
apt_get_update:
  cmd.wait:
    - name: 'apt-get -q update'
    - watch:
      - repo_pkgs
