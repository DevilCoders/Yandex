FROM dbaas-infra-tests-base
LABEL image-cleanup=true

RUN rm -f /etc/supervisor/conf.d/nginx.conf

RUN pip3 install --extra-index-url https://pypi.yandex-team.ru/simple \
    yc-auth-token==0.6.1 grpcio==1.26.0 protobuf==3.19.4 yc_as_mock==0.0.1 yc-as-client==0.0.3

RUN for i in /usr/local/lib/python3.6/dist-packages/yc_as_mock/*.py; \
    do sed -i 's/cloud\.servicecontrol\.proto\.servicecontrol\.v1/yc_as_client\.servicecontrol\.v1/g' $i; \
    done
RUN ln -s /config/identity.conf /etc/supervisor/conf.d/
RUN ln -s /config/access_service.conf /etc/supervisor/conf.d/

EXPOSE 4284 2484 4336
