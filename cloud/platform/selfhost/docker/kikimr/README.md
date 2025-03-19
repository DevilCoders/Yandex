Upgrade:
* fix YDB_VERSION on Dockerfile
* fix --ydb-version on bin/kikimr.sh
* fix version on README.md

Find new version:

    $ apt-cache policy yandex-search-kikimr-kikimr-bin | grep stable-18-6

Run local:    
    
    $ docker run -it -p 2135:2135 --rm --name kikimr container-registry.cloud.yandex.net/crp7nvlkttssi7kapoho/kikimr:stable-18-6 
        
Links:
* https://wiki.yandex-team.ru/kikimr/user/getacluster/localkikimr/
* https://wiki.yandex-team.ru/qloud/docker-registry/
* https://github.yandex-team.ru/Cocaine/dockerfile/tree/master/base
* https://sandbox.yandex-team.ru/task/268191309/view    
    