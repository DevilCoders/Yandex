/etc/ferm/conf.d/20_external_access.conf:
    fs.file_present:
        - contents_function: mdb_firewall.render_external_access_config
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
