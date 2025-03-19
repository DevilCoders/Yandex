FROM ubuntu:18.04

RUN apt-get update && apt-get install -y socat qemu-utils && apt-get clean

RUN mkdir /connect && chmod a+rwx /connect
COPY entrypoint.sh /entrypoint.sh

RUN useradd -m user
USER user:user

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]
CMD ["/bin/bash"]
