/var/lib/locales/supported.d/local:
    file.symlink:
        - target: /usr/share/i18n/SUPPORTED
        - makedirs: true
        - force: true

locale-gen:
    cmd.wait:
        - watch:
            - file: /var/lib/locales/supported.d/local
