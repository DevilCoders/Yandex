zookeeper-pkg 
=============

Yandex ZooKeeper package generator with standard monitorings, init
scripts, etc.

If your service requires ZK and does not yet have a dedicated ZK cluster,
you should create a service-specific .cfg file and build service-specific
packages with it.  See build-zookeeper-pkg.sh for details.

Sharing zk instances between services is not recommended unless you
really know what you're doing and understand operation of the services
in question.

Building
========

Write a <service>.cfg file and run:
$ ./build-zookeeper-pkg.sh <service>

Some service-specific ZK packages are built and duploaded automatically
at mediaservices teamcity cluster: http://teamcity-server.media.dev.yandex.net/project.html?projectId=project239&tab=projectOverview

TODO .cfg customization manual 
TODO build RPMs as well (mostly done)

Administration
==============

There're several Yandex teams using ZooKeeper.
 * media-admin@
 * maps-admin@
 * ...

TODO warn about changing zk hosts

Development
===========

One recommended approach for writing ZK client code in Java is via
Iceberg zk components (please contact us at lambda@yandex-team.ru).

TODO more links
TODO explain REST api
TODO describe iceberg zk components https://hg.yandex-team.ru/media/iceberg

Further questions
=================
Reach us at:
 * http://clubs.at.yandex-team.ru/media/
 * lambda@yandex-team.ru

