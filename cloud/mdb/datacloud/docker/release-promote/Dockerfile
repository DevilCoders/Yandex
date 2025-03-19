FROM cr.yandex/mirror/ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive

COPY build/alerts_checker /alerts_checker
COPY bootstrap.sh /bootstrap.sh

RUN bash /bootstrap.sh

CMD ["/bin/bash"]
