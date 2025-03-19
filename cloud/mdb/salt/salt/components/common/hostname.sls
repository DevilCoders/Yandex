/etc/hostname:
    file.managed:
        - contents: {{ salt['grains.get']('id') }}

set-hostname:
    cmd.wait:
        - name: hostname {{ salt['grains.get']('id') }}
        - watch:
            - file: /etc/hostname
        - order: 1
