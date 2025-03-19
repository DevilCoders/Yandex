data:
    walg:
        ssh_keys:
            private: |
                {{ salt.yav.get('ver-01eka56kcs9bgbky9ya8vd1d16[private]') | indent(16) }}
            pub: {{ salt.yav.get('ver-01eka56kcs9bgbky9ya8vd1d16[public]') }}
