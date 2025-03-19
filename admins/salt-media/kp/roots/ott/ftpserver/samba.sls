samba_config:
  file.managed:
    - name: /etc/samba/smb.conf
    - source: salt://{{ slspath }}/files/etc/samba/smb.conf
    - user: root
    - mode: 644

samba_app:
    pkg.installed:
        - name: samba
    service.running:
        - enable: True
        - name: smbd
        - watch:
            - file: /etc/samba/smb.conf
