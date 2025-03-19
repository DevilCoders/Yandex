FROM nvidia/cuda:11.4.0-runtime-ubuntu18.04

ADD tts_server /
ADD lingware /lingware

ADD remove_speakers /
RUN ./remove_speakers --lingware-path /lingware --speaker-whitelist b1g4atvsitpj2411v2nc

ADD set_default_speaker /
RUN ./set_default_speaker --lingware-path /lingware --speaker b1g4atvsitpj2411v2nc

CMD /tts_server --config-path /lingware/tts_server_config.json --license_service_endpoints $LICENSE_SERVICE_ENDPOINTS --host $SERVICE_HOST --port $SERVICE_PORT  --unistat-port $UNISTAT_PORT
