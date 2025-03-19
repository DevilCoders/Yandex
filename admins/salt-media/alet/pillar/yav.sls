{% set cluster_mapping = {
                           'alet-test-librarian-mongo': 'sec-01czqv43vx71kphdxw1ncwtb95',
                           'alet-stable-librarian-mongo': 'sec-01czqv50ev2027eatpj1za6977',
                         }
%}

{% for key, data in cluster_mapping.items() if key in grains['conductor']['groups'] %}
cluster_yav_secrets: {{ salt.yav.get(data)|json }}
{% endfor %}

project_yav_secrets: {{ salt.yav.get('sec-01czqtjkvdn9gjd36x71a1kw4q')|json }}
