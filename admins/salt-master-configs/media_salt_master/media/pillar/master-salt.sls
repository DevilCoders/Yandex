{% set is_prod = grains['yandex-environment'] == 'production' %}

salt_master:
  lookup:
    arcadia:
      remote_path: 'trunk/arcadia/admins/salt-media/media'
      local_target: '/srv/git/media'
      arc_token: {{ salt.yav.get('sec-01czthpx6yq274a46jpsvzz9ha[arcanum_oauth_token]') | json }}
    monrun:
      memory_threshold: 80 {# % #}
    csync2:
      from_pillar:
        {% if is_prod %}
        csync2.key: {{ salt.yav.get('sec-01cztjf7j32nyrd0q63ff25ngj[production.csync2.key]') | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get('sec-01cztjf7j32nyrd0q63ff25ngj[production.csync2.ssl_crt]') | json }}
        csync2_ssl_key.pem: {{ salt.yav.get('sec-01cztjf7j32nyrd0q63ff25ngj[production.csync2.ssl_key]') | json }}
        {% else %}
        csync2.key: {{ salt.yav.get('sec-01cztjf7j32nyrd0q63ff25ngj[testing.csync2.key]') | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get('sec-01cztjf7j32nyrd0q63ff25ngj[testing.csync2.ssl_crt]') | json }}
        csync2_ssl_key.pem: {{ salt.yav.get('sec-01cztjf7j32nyrd0q63ff25ngj[testing.csync2.ssl_key]') | json }}
        {% endif %}
    ssh:
      {% if is_prod %}
      key: {{ salt.yav.get('sec-01czthpx6yq274a46jpsvzz9ha[production]') | json }}
      {% else %}
      key: {{ salt.yav.get('sec-01czthpx6yq274a46jpsvzz9ha[testing]') | json }}
      {% endif %}
    config: salt://master.conf
    git_local:
      - common:
          git: git@github.yandex-team.ru:salt-media/common.git
