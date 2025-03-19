pt-dl-percona-repo-config:
  pkg.installed:
    - pkgs:
      - percona-repo-config
    - order: 0

pt-dl-pt-packages:
  pkg.installed:
    - refresh: True
    - pkgs:
      - ubic
      - percona-toolkit
