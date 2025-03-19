FROM nvidia/cuda:11.4.0-runtime-ubuntu18.04

RUN apt-get update
RUN apt-get install -y apt-transport-https ca-certificates gnupg

ARG TTS_SERVER_PATH
ARG TTS_LINGWARE_PATH

COPY tts_server /opt/tts_server/
COPY lingware /opt/tts_server/lingware/

CMD /opt/tts_server/tts_server --config-path /opt/tts_server/lingware/tts_server_config.json --license_service_endpoints $LICENSE_SERVICE_ENDPOINTS --host $SERVICE_HOST --port $SERVICE_PORT  --unistat-port $UNISTAT_PORT
