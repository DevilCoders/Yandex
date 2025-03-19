samba:
  pkg.installed

/etc/samba/smb.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/smb.conf

smbd:
  service.running:
    - watch:
      - file: /etc/samba/smb.conf

nmbd:
  service.running:
    - watch:
      - file: /etc/samba/smb.conf
