{% set yaenv = grains['yandex-environment'] %}

{% if yaenv in ["production"] %}
{% set mgsec = "sec-01d72fvzyj3jb259hj65hnfd5m" %}
{% elif yaenv in ["stress"] %}
{% set mgsec = "sec-01d72fwanjjkyae05va98bgqye" %}
{% elif yaenv in ["testing"] %}
{% set mgsec = "sec-01d72fw5t3smvv3y7vea7zkcrg" %}
{% endif %}

privileges: {{ salt.yav.get(mgsec)['item'] | json }}

sudoers:
  kinopoisk-dev-sudo:
    group: "svc_kp_development"
    mail_account: "kinopoisk-sudo"
  kinopoisk-manager-sudo:
    group: "svc_kp_projects_management"
    mail_account: "kinopoisk-sudo"
