ARG PYTHON_VERSION=3.10
ARG PYTHON_IMAGE_VERSION=${PYTHON_VERSION}-slim

FROM cr.yandex/mirror/python:$PYTHON_IMAGE_VERSION

WORKDIR /mr_prober

RUN pip install --upgrade pip

ENV prometheus_multiproc_dir=/tmp/mr-prober-api-prometheus-multiproc
RUN rm -rf $prometheus_multiproc_dir
RUN mkdir -p $prometheus_multiproc_dir

RUN apt-get update && apt-get install -y \
    gcc \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*
#RUN apk add --no-cache postgresql-libs libstdc++
#RUN apk add --no-cache --virtual .build-deps gcc musl-dev postgresql-dev build-base linux-headers libffi-dev

COPY requirements.production.txt requirements.txt ./

RUN pip install -r requirements.production.txt --no-cache-dir

COPY . .

ENV PYTHONPATH=/mr_prober

CMD [ "gunicorn", "-k", "uvicorn.workers.UvicornWorker", "-c", "api/gunicorn_conf.py", "api:app"]
