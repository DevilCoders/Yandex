{% set oscodename = salt['grains.get']('oscodename') %}
{% from slspath + '/map.jinja' import mongodb with context %}

stock-repo:
{% if oscodename in ['xenial'] %}
  pkgrepo.managed:
    - humanname: "MongoDB for 16.04"
    - name: deb [ arch=amd64 ] http://repo.mongodb.org/apt/ubuntu xenial/mongodb-org/3.4 multiverse
    - dist: xenial/mongodb-org/3.4
    - file: /etc/apt/sources.list.d/mongodb.org.list
    - keyid: A15703C6
    - keyserver: hkp://keyserver.ubuntu.com:80
    - order: 144
    - clean_file: true
{% elif "3.2" in mongodb.version %}
  pkgrepo.managed:
    - humanname: "MongoDB for 14.04"
    - name: deb [ arch=amd64 ] http://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/3.2 multiverse
    - dist: trusty/mongodb-org/3.2
    - file: /etc/apt/sources.list.d/mongodb.org.list
    - keyid: EA312927
    - keyserver: hkp://keyserver.ubuntu.com:80
    - clean_file: true
{% elif "3.6" in mongodb.version %}
  pkgrepo.managed:
    - humanname: "MongoDB for 14.04"
    - name: deb [ arch=amd64 ] http://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/3.6 multiverse
    - dist: trusty/mongodb-org/3.6
    - file: /etc/apt/sources.list.d/mongodb.org.list
    - keyid: 91FA4AD5
    - keyserver: hkp://keyserver.ubuntu.com:80
    - clean_file: true
{% else %}
  pkgrepo.managed:
    - humanname: "MongoDB for 14.04"
    - name: deb [ arch=amd64 ] http://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/3.4 multiverse
    - dist: trusty/mongodb-org/3.4
    - file: /etc/apt/sources.list.d/mongodb.org.list
    - keyid: A15703C6
    - keyserver: hkp://keyserver.ubuntu.com:80
    - order: 144
    - clean_file: true
{% endif %}
