percona-repo-config:
  pkg.installed:
    - order: 0

pt-packages:
  pkg.installed:
    - refresh: True
    - pkgs:
      - ubic
      - percona-toolkit