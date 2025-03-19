{% if grains['yandex-environment'] == 'testing' %}
resource-provider-tvm-secret-develop: {{ salt.yav.get('sec-01f8q7m7brm21p4k89kn56pg0z[client_secret]') | json }}
resource-provider-tvm-secret: {{ salt.yav.get('sec-01f9xqzwt1vd5pby7ac1mfgtbd[client_secret]') | json }}
{% else %}
resource-provider-tvm-secret: {{ salt.yav.get('sec-01f8q7nenjvw6qrrmtczb7h24s[client_secret]') | json }}
{% endif %}
