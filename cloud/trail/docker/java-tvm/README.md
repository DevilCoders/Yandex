
How to build docker image with ydb cli
--------------------------------------

* ~~Install Docker on [Mac](https://docs.docker.com/docker-for-mac/install/) / [Windows](https://docs.docker.com/docker-for-windows/install/) / Linux.~~
* Install [Colima](https://github.com/abiosoft/colima)
```bash
brew install colima
colima start
```
* Get [OAuth](https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb) token and make docker login with `docker login -u oauth --password-stdin cr.yandex`. Read more about [Container Registry](https://cloud.yandex.ru/docs/container-registry/operations/authentication)
* Start Docker, if it doesn't start read [help](https://wiki.yandex-team.ru/cloud/devel/container-registry/developer/#mac-18).
* Run script `./docker_build_and_push.sh`. Pushed version of image will be saved to `image.txt`
* Commit `image.txt` changes.
