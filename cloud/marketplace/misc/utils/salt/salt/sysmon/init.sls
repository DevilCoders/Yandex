solomon-sysmond-pkgs:
  pkg.installed:
    - name: yandex-solomon-sysmond
    - version: "1:8.4"

solomon-sysmond-service:
  service.running:
    - name: yandex-solomon-sysmond
    - enable: true
    - require:
      - pkg: solomon-sysmond-pkgs
    - watch:
      - pkg: solomon-sysmond-pkgs
