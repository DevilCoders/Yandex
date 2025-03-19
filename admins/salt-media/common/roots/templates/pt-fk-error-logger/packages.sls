pt-fkel-percona-repo-config:
  pkg.installed:
    - pkgs:
      - percona-repo-config
    - order: 0

pt-fkel-pt-packages:
  pkg.installed:
    - refresh: True
    - pkgs:
      - ubic
      - percona-toolkit
