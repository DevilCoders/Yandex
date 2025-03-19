{% if grains['oscodename'] == 'trusty' %}
yandex-conf-repo-media-trusty-stable:
  pkg.installed
{% endif %}

include:
  - .pkgver-ignore
  - templates.fix-iface-route
  - templates.juggler-search
