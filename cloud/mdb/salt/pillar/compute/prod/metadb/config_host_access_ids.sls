data:
  dbaas_metadb:
    config_host_access_ids:
       - access_id: 34019e42-4c7d-4208-8bde-37cc0430ca3e
         access_secret: {{ salt.yav.get('ver-01e2g6sf1zyys5kbdc1h83kdtw[hash]') }}
         active: True
         type: 'default'
       - access_id: 3443b6da-788f-4144-846b-529d4a0449f0
         access_secret: {{ salt.yav.get('ver-01e2g6yyp7t2m3vkxn47f2at08[hash]') }}
         active: True
         type: 'dbaas-worker'
       - access_id: 9156e5e5-ec3f-4a02-9d95-9cc74aaf98d0
         access_secret: {{ salt.yav.get('ver-01e2g7279c4dvage0nysecpg02[hash]') }}
         active: True
         type: 'dataproc-api'
       - access_id: {{ salt.yav.get('ver-01edxck0hfncb83qcmdz70a0xn[id]') }}
         access_secret: {{ salt.yav.get('ver-01edxck0hfncb83qcmdz70a0xn[hash]') }}
         active: True
         type: 'dataproc-ui-proxy'
