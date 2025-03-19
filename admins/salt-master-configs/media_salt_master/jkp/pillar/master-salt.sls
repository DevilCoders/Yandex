{% set is_prod = grains['yandex-environment'] == 'production' %}
{% set sec_id = 'sec-01d6043n6m1v5ta8pzrt0ay3z7' if is_prod else 'sec-01d60459qrsea36qcgcfppkvds' %}

salt_master:
  lookup:
    arcadia: 
      remote_path: 'trunk/arcadia/admins/salt-media/jkp'
      local_target: '/srv/git/jkp'
      arc_token: {{ salt.yav.get('sec-01czthpx6yq274a46jpsvzz9ha[arcanum_oauth_token]') | json }}
    csync2:
      from_pillar:
        csync2.key: {{ salt.yav.get(sec_id + '[csync2.key]') | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get(sec_id + '[csync2_ssl_cert.pem]') | json }}
        csync2_ssl_key.pem: {{ salt.yav.get(sec_id + '[csync2_ssl_key.pem]') | json }}
    ssh:
        key: {{ salt.yav.get(sec_id + '[robot_media_salt_id_rsa]') | json }}
    config: salt://master.conf
    git_local:
        - common:
            git: git@github.yandex-team.ru:salt-media/common.git
