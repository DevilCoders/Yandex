FROM nvidia/cuda:11.3.0-runtime-ubuntu20.04

ADD tts_server /
ADD lingware /lingware

CMD [ "/bin/sh", "-c", "./tts_server --config-path /lingware/tts_server_config.json --port 17004 --unistat-port 17002 2>&1" ]

