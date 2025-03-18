ARG arch=amd64
FROM --platform=linux/${arch} docker.elastic.co/elasticsearch/elasticsearch:8.3.2

COPY ./ /

USER root

RUN chmod -R 777 /launch /solomon-agent
RUN chmod g+w /usr/share/elasticsearch/config
