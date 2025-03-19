FROM nvidia/cuda:10.1-runtime-ubuntu16.04
ARG port=17002

ADD run.sh /
ADD run_service /
ADD normalizer /
EXPOSE $port
CMD [ "/bin/bash", "run.sh" ]
