lazy-trim-pkg:
    pkg.installed:
        - pkgs:
            - yandex-lazy-trim: '1.8722366'
        - prereq_in:
            - cmd: repositories-ready
