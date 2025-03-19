PROFILE_PREPROD=preprod
PROFILE_PROD=prod

K8S_ID_PREPROD=c49mgjfnkj8q8fq3obpj
K8S_ID_PROD=catojiho8te843kmhrk8


docker-${PROFILE_PREPROD}:
	yc container registry configure-docker --profile connman-${PROFILE_PREPROD} --hostname cr.cloud-preprod.yandex.net --hostname container-registry.cloud-preprod.yandex.net
	cd docker; \
PROFILE=${PROFILE_PREPROD} \
DOCKER_REGISTRY=cr.cloud-preprod.yandex.net \
DOCKER_REPOSITORY=crt73rt5m8j28oujjv2c \
source build.sh

docker-${PROFILE_PROD}:
	yc container registry configure-docker --profile connman-${PROFILE_PROD}
	cd docker; \
PROFILE=${PROFILE_PROD} \
DOCKER_REGISTRY=cr.yandex \
DOCKER_REPOSITORY=crpb5l8iv58ost7b3imq \
source build.sh


k8s-deploy-${PROFILE_PREPROD}:
	cd k8s; \
PROFILE=${PROFILE_PREPROD} \
K8S_ID=${K8S_ID_PREPROD} \
source deploy.sh

k8s-deploy-${PROFILE_PROD}:
	cd k8s; \
PROFILE=${PROFILE_PROD} \
K8S_ID=${K8S_ID_PROD} \
source deploy.sh


k8s-install-${PROFILE_PREPROD}:
	cd k8s; \
PROFILE=${PROFILE_PREPROD} \
K8S_ID=${K8S_ID_PREPROD} \
DOMAIN=connman.cloud-preprod.yandex.net \
CERTIFICATE_ID=fd3b81rkcf4jeq7hqc2d \
source install.sh

k8s-install-${PROFILE_PROD}:
	cd k8s; \
PROFILE=${PROFILE_PROD} \
K8S_ID=${K8S_ID_PROD} \
DOMAIN=connman.cloud.yandex.net \
CERTIFICATE_ID=fpqpqrrjnhacj528rgrp \
source install.sh


# https://stackoverflow.com/a/6273809/625648
tf-${PROFILE_PREPROD}:
	cd terraform; \
PROFILE=${PROFILE_PREPROD} \
DB_ADMIN_PASSWORD_SECRET=sec-01fnc17vreb79zz8ws6fe3fv1v \
source tf.sh $(filter-out $@, $(MAKECMDGOALS))

tf-${PROFILE_PROD}:
	cd terraform; \
PROFILE=${PROFILE_PROD} \
DB_ADMIN_PASSWORD_SECRET=sec-01fvybdbkxvcwwgqbk4cj0pz1s \
source tf.sh $(filter-out $@, $(MAKECMDGOALS))

%:
	@:
