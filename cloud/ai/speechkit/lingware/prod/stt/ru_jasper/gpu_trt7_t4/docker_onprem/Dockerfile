FROM nvidia/cuda:11.4.0-runtime-ubuntu18.04

RUN apt-get update
RUN apt-get install -y apt-transport-https ca-certificates gnupg

ADD asr_server /
ADD lingware /lingware

CMD /asr_server --config-path /lingware/asr_system_config.json --license_service_endpoints $LICENSE_SERVICE_ENDPOINTS --host $SERVICE_HOST --port $SERVICE_PORT --unistat-port $UNISTAT_PORT
