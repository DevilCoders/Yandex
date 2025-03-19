# HOWTO: Elasticsearch plugins package

### Скачиваем новые плагины с elastic.co

Допустим мы хотим собрать пакет с плагинами для версии 7.11.1, выполняем команду

	version=7.11.1 ./download-packages.sh

и мы получим каталог `plugins-7.11.1` с плагинами 

	plugins-7.11.1/
	├── analysis-icu.zip
	├── analysis-kuromoji.zip
	├── analysis-nori.zip
	├── analysis-phonetic.zip
	├── analysis-smartcn.zip
	├── analysis-stempel.zip
	├── analysis-ukrainian.zip
	├── discovery-azure-classic.zip
	├── discovery-ec2.zip
	├── discovery-gce.zip
	├── ingest-attachment.zip
	├── mapper-annotated-text.zip
	├── mapper-murmur3.zip
	├── mapper-size.zip
	├── repository-azure.zip
	├── repository-gcs.zip
	├── repository-hdfs.zip
	├── repository-s3.zip
	├── store-smb.zip
	└── transport-nio.zip
	
Заменяем файл `repository-s3.zip` на пропатченый `repository-s3.zip`, который нужно собрать по [инструкции](./repository-s3.md)

### Загружаем наши плагины как ресурс sandbox

	ya upload --tar --ttl=inf plugins-7.11.1

### Сборка пакета
	
Создаем файл `plugins-7.11.1.json` следующего содержания (со своей версией, путем и id ресурса)

	{
	    "meta": {
	        "name": "mdb-elasticsearch-plugins",
	        "maintainer": "mdb <mdb@yandex-team.ru>",
	        "description": "mdb elasticsearch plugins",
	        "version": "7.11.1",
	        "depends": [],
	        "noconffiles_all": true,
	        "homepage": "http://elastic.co/"
	    },
	    "data": [
	        {
	            "source": {
	                "type": "SANDBOX_RESOURCE",
	                "path": "plugins-7.11.1",
	                "id": 2097579487,
	                "untar": true
	            },
	            "destination": {
	                "path": "/opt/yandex/mdb-elasticsearch-plugins",
	            }
	        }
	    ]
	}

Коммитим его и запускаем джобу https://teamcity.aw.cloud.yandex.net/buildConfiguration/MDB_ElasticsearchPluginsArc в параметрах указываем `custom-version: 7.11.1`
