FROM nvidia/cuda:10.1-runtime-ubuntu16.04
ARG port=17002

ADD run.sh /
ADD asr-server /
ADD asr-server.xml /
ADD remote_nodes_preprod.json /
ADD remote_nodes_prod.json /

EXPOSE $port
CMD [ "/bin/bash", "run.sh" ]
