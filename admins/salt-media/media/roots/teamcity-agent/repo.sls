docker-stock-repo:
  pkgrepo.managed:
    - humanname: "Docker repo"
    - name: deb [arch=amd64] https://download.docker.com/linux/ubuntu trusty stable
    - file: /etc/apt/sources.list.d/docker.list
    - keyid: 7EA0A9C3F273FCD8
    - keyserver: keyserver.ubuntu.com
    - order: 1

cassandra-stock-repo:
  pkgrepo.managed:
    - humanname: "Cassandra 3.9 from mainline"
    - name: deb http://www.apache.org/dist/cassandra/debian 39x main
    - dist: 39x
    - file: /etc/apt/sources.list.d/apache.cassandra.list
    - keyid: A278B781FE4B2BDA
    - keyserver: keys.openpgp.org
    - order: 2

mongodb-stock-repo:
  pkgrepo.managed:
    - humanname: "MongoDB for 14.04"
    - name: deb http://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/3.6 multiverse
    - dist: trusty/mongodb-org/3.6
    - file: /etc/apt/sources.list.d/mongodb-org.list
    - keyid: 91FA4AD5
    - keyserver: keyserver.ubuntu.com
    - order: 3

mysql-repcona-repo:
  pkgrepo.managed:
    - humanname: "Mysql Percona repo"
    - name: deb http://mirror.yandex.ru/mirrors/percona/percona/apt trusty main
    - file: /etc/apt/sources.list.d/percona-repo-config.list
    - keyid: 9334A25F8507EFA5
    - keyserver: keys.gnupg.net
    - order: 4

ondrej-php-trusty-repo:
  pkgrepo.managed:
    - humanname: "php for trusty (14.04) repo"
    - name: deb http://gameoptic.com/mirror/trusty/ppa.launchpad.net/ondrej/php/ubuntu trusty main
    - file: /etc/apt/sources.list.d/ondrej-php-trusty.list
    - order: 5

repo_packages:
  pkg.installed:
    - pkgs:
      - yandex-conf-repo-testing
      - yandex-conf-repo-unstable
      - yandex-conf-repo-prestable
    - order: 6

