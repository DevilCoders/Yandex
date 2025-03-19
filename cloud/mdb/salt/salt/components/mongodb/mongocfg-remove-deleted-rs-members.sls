mongocfg-remove-deleted-rs-members:
    mongodb.replset_remove_deleted:
        - port: 27019
        - user: admin
        - authdb: admin
        - password: {{ salt.pillar.get('data:mongodb:users:admin:password')|yaml_encode }}
        - host: {{ salt.grains.get('fqdn') }}
        - replset_hosts: {{ salt.pillar.get('data:dbaas:cluster_hosts') }}
