`mkdir ~/arc/arcadia/cloud/ai/selfhost/docker/vision-server/mamba/configs`

Если берём не кастомные конфиги из мэджика, а стандартные для мамбы из аркадии, например, для нужд тестирования образа, то нужно будет поменять конфиги руками:

`cd ~/arc/arcadia/voicetech/asr/cloud_engine/server/vision`

`ya make`

`cp launch_cbirdaemon2/data/cbir_3heads.conf ~/arc/arcadia/cloud/ai/selfhost/docker/vision-server/mamba/configs/cbir_data.conf`

`cp launch_cbirdaemon2/data/graph_3heads.pb ~/arc/arcadia/cloud/ai/selfhost/docker/vision-server/mamba/configs/graph.pb`

`cp launch_cbirdaemon2/prod_configs/cbirdaemon_3heads.conf ~/arc/arcadia/cloud/ai/selfhost/docker/vision-server/mamba/configs/cbir_prod_config.conf`

`cp python/server/config.json ~/arc/arcadia/cloud/ai/selfhost/docker/vision-server/mamba/configs/config.json`

`cd ~/arc/arcadia/cloud/ai/selfhost/docker/vision-server/mamba`

залезаем в файлик `docker/vision-server/mamba/configs/cbir_data.conf`, в нём меняем `Config: "graph_3heads.pb"` на `Config: "graph.pb"`
залезаем в файлик `docker/vision-server/mamba/configs/cbir_prod_config.conf`, в нём меняем `ConfigName: "cbir_3heads.conf"` на `ConfigName: "cbir_data.conf"`

Если же берём конфиги из мэджика, то просто копируем соответствующие файлы из выхода мэджика, аналогично раскладываем их в папку `configs`, но менять в файликах ничего не нужно, так как мэджики уже генерят правильные конфиги под конкретного клиента и его датасет

`ya package docker_package.json --docker --docker-registry cr.yandex --docker-repository crppns4pq490jrka0sth`

Поднимаем контейнер из образа: `docker run -it --privileged --network host %image%` или `docker run -dit --privileged --network host %image%` для detached-mode.
