install_suricata:
    pkg.installed:
      - pkgs:
          - suricata


enable_suricate_service:
    service.running:
      - name: suricata
      - enable:  True
      - watch:
          - pkg: install_suricata