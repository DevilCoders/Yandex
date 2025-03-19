FROM tensorflow-serving-mkl:latest

EXPOSE 80

WORKDIR /opt/application/

RUN mkdir -p /models/badoo
RUN mkdir -p /opt/application/

# libtiff5-dev is for pillow-simd
RUN apt-get update \
    && apt-get install -y curl gnupg2 python3-dev python3-pip supervisor libtiff5-dev

RUN CC="cc -mavx2" pip3 install -U --force-reinstall pillow-simd
RUN pip3 install tensorflow tensorflow-serving-api keras tornado flask uwsgi

RUN apt-get update; apt-get install -y nginx
ADD docker/nginx.conf /etc/nginx/

COPY docker/supervisor/ /etc/supervisor/conf.d

COPY model/ /models/badoo
ADD models.conf /models/

COPY rest_server.py /opt/application

ADD docker-entry.sh uwsgi.ini /
CMD [ "bash", "/docker-entry.sh" ]
