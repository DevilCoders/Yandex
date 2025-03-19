FROM nvidia/cuda:10.1-runtime-ubuntu16.04

ARG port=17004

ADD tts-server /
RUN chmod +x tts-server

ADD evlogdump /
RUN chmod +x evlogdump

ADD lingware/ru_valtz_e2e /lingware/ru_valtz_e2e

ADD run.sh /
RUN chmod +x run.sh

VOLUME ["/logs"]

EXPOSE $port
CMD ./run.sh > /logs/tts_server.log
