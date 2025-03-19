force-reload-ferm-rules:
    test.succeed_with_changes:
        - watch_in:
            - cmd: reload-ferm-rules
