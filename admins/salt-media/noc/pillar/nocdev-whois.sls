{% set sec_id = 'sec-01fmn0w2jegdrzmxqhr2ttp880' %}
sec: {{ salt.yav.get(sec_id)|json }}
unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g5zvfdxtqb257hsc1qhy5mcb')['client_secret']|json}}

postgresql:
  version: 14
  cluster: main
  config:
    max_connections: '256'
    wal_log_hints: 'on'
    listen_addresses: "'*'"
    shared_buffers: '24GB'
    effective_cache_size: '72GB'
    maintenance_work_mem: '2GB'
    checkpoint_completion_target: '0.9'
    wal_buffers: '32MB'
    work_mem: '512MB'
    wal_keep_size: '32768'
  hba:
{% for host in salt.conductor.groups2hosts(grains['conductor']['group']) %}
   - type: host
     user: all
     db: all
     address: {{ salt['cmd.shell']('host -t AAAA ' + host + '| awk "{ print \$NF }"') }}/64
     method: md5
   - type: host
     user: all
     db: replication
     address: {{ salt['cmd.shell']('host -t AAAA ' + host + '| awk "{ print \$NF }"') }}/64
     method: md5
{% endfor %}
  users:
  - user: repl
    password: {{ salt.yav.get(sec_id + '[pg_repl_password]')|json }}
    options:
      - LOGIN
      - REPLICATION
  - user: repmgr
    password: {{ salt.yav.get(sec_id + '[pg_repmgr_password]')|json }}
    options:
      - LOGIN
      - SUPERUSER
  - user: irrd
    password: {{ salt.yav.get(sec_id + '[pg_irrd_password]')|json }}
    options:
      - LOGIN
  repmgr:
    node_id: {{ salt.conductor.groups2hosts(grains['conductor']['group']).index(grains['fqdn'])+1 }}
    ssh_key: {{ salt.yav.get(sec_id + '[ssh_postgres]')|json }}
    user: repmgr
    password: {{ salt.yav.get(sec_id + '[pg_repmgr_password]')|json }}
