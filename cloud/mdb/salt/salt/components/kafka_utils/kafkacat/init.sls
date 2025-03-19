kafkacat-pkgs:
    pkg.installed:
        - pkgs:
            - kafkacat: '1.3.1-1'
        - prereq_in:
            - cmd: repositories-ready
