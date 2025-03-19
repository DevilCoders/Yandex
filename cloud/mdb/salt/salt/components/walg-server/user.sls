walg-user:
    user.present:
        - fullname: WAL-G system user
        - name: walg
        - createhome: True
        - empty_password: False
        - system: True

/home/walg/.ssh/authorized_keys:
    file.managed:
        - contents_pillar: 'data:walg:ssh_keys:pub'
        - user: walg
        - group: walg
        - makedirs: True
        - require:
            - user: walg-user
