{% if grains['yandex-environment'] in ['testing'] %}
nginx_mulcagate:
  slb: storage.stm.yandex.net storagetest.mail.yandex.net
  ssl_certificate: /etc/yandex-certs/storage.stm.yandex.net.crt
  ssl_certificate_key: /etc/yandex-certs/storage.stm.yandex.net.pem
  tvm_clients:
    - 186
    - 184
    - 2000376
{% else %}
nginx_mulcagate:
  slb: storage.mail.yandex.net
  ssl_certificate: /etc/yandex-certs/storage.mail.yandex.net.crt
  ssl_certificate_key: /etc/yandex-certs/storage.mail.yandex.net.pem
  tvm_clients:
    - 185
    - 249
    - 167
    - 178
    - 2000031
    - 2000377
{% endif %}
