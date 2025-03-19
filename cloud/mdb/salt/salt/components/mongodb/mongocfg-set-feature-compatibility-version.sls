mongocfg-set-feature-compatibility-version:
    mongodb.feature_compatibility_version:
        - name: {{ salt.grains.get('fqdn') }}
        - fcv: '{{ salt.pillar.get('data:mongodb:feature_compatibility_version', '4.0') }}'
        - user: admin
        - password: {{ salt.pillar.get('data:mongodb:users:admin:password')|yaml_encode }}
        - port: 27019
        - authdb: admin
