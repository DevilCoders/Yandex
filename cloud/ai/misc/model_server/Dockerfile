FROM ubuntu:18.04

EXPOSE 80

WORKDIR /opt/application/

RUN mkdir -p /models/badoo
RUN mkdir -p /opt/application/

# libtiff5-dev is for pillow-simd
RUN apt-get update \
    && apt-get install -y curl gnupg2 python3-dev python3-pip supervisor libtiff5-dev nginx

# Add Tensorflow Serving package source, https://www.tensorflow.org/serving/setup
RUN echo "deb [arch=amd64] http://storage.googleapis.com/tensorflow-serving-apt stable tensorflow-model-server tensorflow-model-server-universal" \
    | tee /etc/apt/sources.list.d/tensorflow-serving.list \
    && curl https://storage.googleapis.com/tensorflow-serving-apt/tensorflow-serving.release.pub.gpg \
    | apt-key add -

RUN apt-get update \
    && apt-get install -y tensorflow-model-server \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN CC="cc -mavx2" pip3 install -U --force-reinstall pillow-simd
RUN pip3 install tensorflow tensorflow-serving-api tornado flask

RUN export PYTHONPATH=/opt/application/

COPY docker/supervisor/ /etc/supervisor/conf.d

COPY models/ /models/
ADD docker/models.conf /models/
ADD docker/nginx.conf /etc/nginx/

COPY src/model_server/app /opt/application

ADD docker-entry.sh /
CMD [ "bash", "/docker-entry.sh" ]
