FROM ubuntu:16.04
ARG server_port=50052
ARG cbirdaemon_port=11122

ADD vision /vision
ADD cbirdaemon2 /cbirdaemon2
ADD launch.sh launch.sh

EXPOSE $server_port
EXPOSE $cbirdaemon_port
# https://docs.docker.com/config/containers/multi-service_container/
CMD bash launch.sh

# how to run container from image ?
# docker run -it --privileged --network host %image%
