
How to build docker image with ydb cli
--------------------------------------

* Install Docker on [Mac](https://docs.docker.com/docker-for-mac/install/) / [Windows](https://docs.docker.com/docker-for-windows/install/) / Linux.
* Get iam token
`yc iam create-token --profile prod-fed`
* Make docker login with `docker login --username iam --password-stdin cr.yandex`
Read more about [Container Registry](https://cloud.yandex.ru/docs/container-registry/operations/authentication)
* Start Docker, if it doesn't start read [help](https://wiki.yandex-team.ru/cloud/devel/container-registry/developer/#mac-18).
* Run script `./docker_build_and_push.sh`. Pushed version of image will be saved to `image.txt`
* Commit `image.txt` changes.
