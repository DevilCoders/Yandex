ARG PYTHON_VERSION=3.10
ARG PYTHON_IMAGE_VERSION=${PYTHON_VERSION}-alpine

FROM python:$PYTHON_IMAGE_VERSION

WORKDIR /mr_prober

RUN apk update

RUN python -m pip install --upgrade pip

RUN apk add --no-cache postgresql-libs libstdc++
# See https://github.com/pyca/cryptography/blob/main/docs/installation.rst#alpine
RUN apk add --no-cache --virtual .build-deps gcc musl-dev postgresql-dev build-base linux-headers libffi-dev libressl-dev

COPY requirements.production.txt requirements.txt ./

RUN pip install -r requirements.production.txt --no-cache-dir

RUN apk --purge del .build-deps

COPY . .

ENV PYTHONPATH=/mr_prober

ENTRYPOINT [ "python", "/mr_prober/tools/synchronize/main.py" ]
