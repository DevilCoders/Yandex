ARG PYTHON_VERSION=3.10
ARG PYTHON_IMAGE_VERSION=${PYTHON_VERSION}-slim

FROM cr.yandex/mirror/python:$PYTHON_IMAGE_VERSION

WORKDIR /mr_prober

RUN apt-get update && apt-get install -y \
    cgroup-tools \
    curl \
    dnsutils \
    gcc \
    iputils-ping \
    iptables \
    iproute2 \
    net-tools \
    netcat \
    jq \
    traceroute \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --upgrade pip

COPY requirements.txt .

RUN pip install -r requirements.txt --no-cache-dir

COPY . .

ENV PYTHONPATH=/mr_prober

CMD [ "python", "/mr_prober/agent/main.py" ]
