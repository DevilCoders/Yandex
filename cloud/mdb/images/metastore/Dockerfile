# parametrized to override in preprod
# prod    cr.yandex/crp1ctmeteesp965qmg5/metastore-server-base:latest
# preprod cr.cloud-preprod.yandex.net/crt8jctksovm5j6sf62t/metastore-server-base:latest
ARG BASE_IMAGE=cr.yandex/crp1ctmeteesp965qmg5/metastore-server-base:latest
FROM $BASE_IMAGE
ENV JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
ENV HADOOP_HOME=/opt/hadoop
RUN mkdir -p ~/.postgresql && \
wget "https://storage.yandexcloud.net/cloud-certs/CA.pem" -O ~/.postgresql/root.crt && \
chmod 0600 ~/.postgresql/root.crt
COPY metastore-site.xml /opt/metastore/conf/metastore-site.xml
# docker build . --tag cr.yandex/crp1ctmeteesp965qmg5/metastore-server:latest
# docker run --publish 9083:9083 --detach --name metastore-server cr.yandex/crp1ctmeteesp965qmg5/metastore-server:latest
# docker push cr.yandex/crp1ctmeteesp965qmg5/metastore-server:latest
# docker exec -it <mycontainer> bash
CMD /opt/metastore/bin/start-metastore
