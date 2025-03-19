FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install nano wget openjdk-17-jre-headless -y
ENV JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
RUN wget https://dlcdn.apache.org/hive/hive-standalone-metastore-3.0.0/hive-standalone-metastore-3.0.0-bin.tar.gz \
 && tar xzf hive-standalone-metastore-3.0.0-bin.tar.gz \
 && rm -f hive-standalone-metastore-3.0.0-bin.tar.gz \
 && mv apache-hive-metastore-3.0.0-bin /opt/metastore \
 && wget https://jdbc.postgresql.org/download/postgresql-42.3.1.jar --directory-prefix=/opt/metastore/lib/ \
 && wget https://dlcdn.apache.org/hadoop/common/hadoop-3.3.1/hadoop-3.3.1.tar.gz \
 && tar xzf hadoop-3.3.1.tar.gz \
 && rm -f hadoop-3.3.1.tar.gz \
 && mv hadoop-3.3.1 /opt/hadoop
ENV HADOOP_HOME=/opt/hadoop
# docker build . --tag cr.yandex/crp1ctmeteesp965qmg5/metastore-server-base:latest
RUN useradd metastore
