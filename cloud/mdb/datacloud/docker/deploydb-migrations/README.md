../create_users.py -c 'host=deploydb port=5432 dbname=deploydb user=deploy_admin' -a deploy -g ./grants/
pgmigrate migrate -t latest
