FROM nvidia/cuda:11.3.0-runtime-ubuntu20.04
ARG port=17004

ADD run.sh /
ADD asr_server /
ADD lingware /lingware
EXPOSE $port
CMD [ "/bin/bash", "run.sh" ]
