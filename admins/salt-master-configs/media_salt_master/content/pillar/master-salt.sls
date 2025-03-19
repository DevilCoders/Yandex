{% set is_prod = grains['yandex-environment'] == 'production' %}
{% set secret = 'sec-01d5p4y6bmew51682bzkszycph' if is_prod else 'sec-01d5pj63a69s7cyp01rbmwy0ff' %}

salt_master:
  lookup:
    arcadia:
      remote_path: 'trunk/arcadia/admins/salt-media/content'
      local_target: '/srv/git/content'
      arc_token: {{ salt.yav.get('sec-01czthpx6yq274a46jpsvzz9ha[arcanum_oauth_token]') | json }}
    csync2:
      from_pillar:
        csync2.key: {{ salt.yav.get(secret)['csync2.key'] | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get(secret)['csync2_ssl_cert.pem'] | json }}
        csync2_ssl_key.pem: {{ salt.yav.get(secret)['csync2_ssl_key.pem'] | json }}
    ssh:
        key: {{ salt.yav.get(secret)['robot_media_salt_id_rsa'] | json }}
    config: salt://master.conf
    git_local:
        - common:
            git: git@github.yandex-team.ru:salt-media/common.git
