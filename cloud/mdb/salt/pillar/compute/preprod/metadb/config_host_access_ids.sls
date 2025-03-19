data:
  dbaas_metadb:
    config_host_access_ids:
       - access_id: "34019e42-4c7d-4208-8bde-37cc0430ca3e"
         access_secret: {{ salt.yav.get('ver-01dw7qkq5wt078nf0gf3s8kzss[hash]') }}
         active: True
         type: 'default'
       - access_id: {{ salt.yav.get('ver-01dw7qq43p84ss4hwpwcz77d1d[id]') }}
         access_secret: {{ salt.yav.get('ver-01dw7qq43p84ss4hwpwcz77d1d[hash]') }}
         active: True
         type: 'dbaas-worker'
       - access_id: {{ salt.yav.get('ver-01dw7qv4nb107cn5cvy9k7052m[id]') }}
         access_secret: {{ salt.yav.get('ver-01dw7qv4nb107cn5cvy9k7052m[hash]') }}
         active: True
         type: 'dataproc-api'
       - access_id: {{ salt.yav.get('ver-01edxbx92r3cwnag1bnj3mhnd3[id]') }}
         access_secret: {{ salt.yav.get('ver-01edxbx92r3cwnag1bnj3mhnd3[hash]') }}
         active: True
         type: 'dataproc-ui-proxy'
