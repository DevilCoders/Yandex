FROM ubuntu:bionic

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update

RUN apt-get install -y git curl libicu60 nuget

RUN git clone https://github.com/emgarten/Sleet.git /opt/sleet

RUN cd /opt/sleet && ./build.sh

RUN cp -r /opt/sleet/artifacts/publish/net6.0 /Sleet

RUN cp -r /opt/sleet/.cli /Dotnet

RUN rm -rf /root/.config /root/.nuget /opt/sleet

ENV DOTNET_ROOT=/Dotnet

VOLUME /package

ADD entrypoint.sh /entrypoint.sh

RUN chmod a+x entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
