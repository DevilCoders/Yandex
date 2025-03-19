FROM nvidia/cuda:11.3.0-runtime-ubuntu16.04
ARG port=17004

ADD tts_server /
ADD lingware /lingware

EXPOSE $port
CMD [ "/bin/sh", "-c", "touch /var/log/tts_server.log && cp /var/log/tts_server.log /var/log/tts_server.log.bac && ./tts_server --config-path /lingware/tts_server_config.json --port 17004 >> /var/log/tts_server.log 2>&1" ]

