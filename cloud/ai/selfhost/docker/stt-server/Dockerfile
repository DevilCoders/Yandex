FROM nvidia/cuda:9.1-runtime

ARG port=17002
ARG model=ru_dialog_general_e2e

ADD normalizer /normalizer/
ADD $model /$model
ADD asr-server /
ADD asr-server.xml /
EXPOSE $port
CMD [ "/bin/sh", "-c", "TF_CUDNN_USE_AUTOTUNE=0; OMP_NUM_THREADS=1; ./asr-server asr-server.xml > /var/log/asr/server.log 2>&1" ]
