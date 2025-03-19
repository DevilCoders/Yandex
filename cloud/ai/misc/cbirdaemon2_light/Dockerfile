FROM ubuntu:18.04

ARG port=15522
ARG threads=10

COPY cbirdaemon2_light app/
RUN chmod 700 app/cbirdaemon2_light

EXPOSE $port

ENV envport=$port
ENV envthreads=$threads
CMD app/cbirdaemon2_light --port ${envport} --threads ${envthreads}
