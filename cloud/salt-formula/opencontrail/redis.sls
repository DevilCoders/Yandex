/etc/redis/redis.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/redis/redis.conf
    - template: jinja

opencontrail_redis_packages:
  yc_pkg.installed:
    - pkgs:
      - redis-server
    - hold_pinned_pkgs: True
    - require:
      - file: /etc/redis/redis.conf

# Restart Redis if we changed the configuration.
# Necessary if both web ui and analytics run on the same host.
redis-server:
  service.running:
    - enable: True
    - require:
      - file: /etc/redis/redis.conf
      - yc_pkg: opencontrail_redis_packages
    - watch:
      - file: /etc/redis/redis.conf
