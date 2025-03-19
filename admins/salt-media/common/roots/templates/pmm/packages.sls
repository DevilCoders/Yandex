percona-repo-config_pmm:
  pkg.installed:
    - pkgs:
      - percona-repo-config
    - order: 0

pmm-packages:
  pkg.installed:
    - refresh: True
    - pkgs:
      - pmm-client
