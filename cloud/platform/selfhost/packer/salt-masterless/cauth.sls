cauth package:
  pkg.installed:
  - pkgs:
    - yandex-cauth

allow idm rules:
  file.managed:
    - name: /etc/cauth/cauth.conf
    - contents: |
        sources=conductor,idm
