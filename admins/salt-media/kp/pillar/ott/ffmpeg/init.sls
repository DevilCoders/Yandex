include:
  - {{ slspath }}.vncserver
  - {{ slspath }}.s3-secure

{% if grains['oscodename'] == 'trusty' %}
docker:
  version: '17.09.1~ce-0~ubuntu'
{% endif %}
