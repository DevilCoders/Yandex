rsyncd:
  lookup:
    shares:
      'mongo-backup':
        - 'read only': 'no'
        - 'path': '/opt/storage/mongo'
        - 'comment': 'Backups for mongo bases'
      'solr-backup':
        - 'read only': 'no'
        - 'path': '/opt/storage/solr'
        - 'comment': 'Backups for solr indexes'
      'pg-backup':
        - 'uid' : 'barman'
        - 'gid' : 'barman'
        - 'read only': 'no'
        - 'path': '/opt/storage/pg'
        - 'comment': 'Backups for postgres bases'
      'all-backups':
        - 'read only': 'no'
        - 'path': '/opt/storage'
        - 'comment': 'All backups, need for backup-backup ;)'
    mopts:
      type: {{ salt['grains.get']('conductor:project') }}
