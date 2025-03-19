{% set is_prod = grains['yandex-environment'] == 'production' %}

salt_master:
  lookup:
    csync2:
      from_pillar:
        {% if is_prod %}
        csync2.key: {{ salt.yav.get('sec-01d56xjbsqfvkhyfwykgw8dpcg[csync2.key]') | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get('sec-01d56xjbsqfvkhyfwykgw8dpcg[csync2_ssl_cert.pem]') | json }}
        csync2_ssl_key.pem: {{ salt.yav.get('sec-01d56xjbsqfvkhyfwykgw8dpcg[csync2_ssl_key.pem]') | json }}
        {% else %}
        csync2.key: {{ salt.yav.get('sec-01d56xfy5yw2m81bg062g2y50b[csync2.key]') | json }}
        csync2_ssl_cert.pem: {{ salt.yav.get('sec-01d56xfy5yw2m81bg062g2y50b[csync2_ssl_cert.pem]') | json }}
        csync2_ssl_key.pem: {{ salt.yav.get('sec-01d56xfy5yw2m81bg062g2y50b[csync2_ssl_key.pem]') | json }}
        {% endif %}
    ssh:
      {% if is_prod %}
      key: {{ salt.yav.get('sec-01d56xjbsqfvkhyfwykgw8dpcg[id_rsa]') | json }}
      {% else %}
      key: {{ salt.yav.get('sec-01d56xfy5yw2m81bg062g2y50b[id_rsa]') | json }}
      {% endif %}
    config: salt://master.conf
    git_local:
        - common:
            git: git@github.yandex-team.ru:salt-media/common.git
        - mediabilling:
            git: git@github.yandex-team.ru:salt-media/mediabilling.git
