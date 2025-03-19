{% if "mongodb" in pillar %}
#####################[start]#####################

{% set mongodb = salt.pillar.get("mongodb") %}

include:
  - templates.mongodb.common
  - templates.mongodb.replicaset

# cleanup
mongodbcfg_disabled:
  service.dead:
    - name: mongodbcfg
    - enable: False


# cleanup files from packages
mongodb_cleanup:
  file.absent:
    - names:
      - /etc/mongos-default.conf
      - /etc/mongodb-default.conf
      - /etc/mongodbcfg-default.conf
      - /etc/monrun/conf.d/mongos.conf
      - /etc/monrun/conf.d/mongo.conf
      - /etc/monrun/conf.d/mongo-health.conf
######################[end]######################
{% else %}
mongodb pillar NOT DEFINED:
  cmd.run:
    - name: echo "may be memory leaks on salt master!!!";exit 1
{% endif %}
