compute-pkgs:
    pkg.installed:
        - pkgs:
            - acpid
        - prereq_in:
            - cmd: repositories-ready
