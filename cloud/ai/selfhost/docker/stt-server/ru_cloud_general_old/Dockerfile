FROM nvidia/cuda:10.1-runtime-ubuntu16.04
ARG port=17002
ARG model=ru_cloud_general_e2e

ADD $model /$model
ADD $model/normalizer.tar.gz /$model
ADD asr-server /
ADD asr-server.xml /
EXPOSE $port
CMD [ "/bin/sh", "-c", "GRAPH_CACHE_ENABLED=false ./asr-server asr-server.xml > /var/log/asr/server.log 2>&1" ]
