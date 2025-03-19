build:
	docker build -t registry.yandex.net/junk/automl-image-prototype:latest .

push:
	docker push registry.yandex.net/junk/automl-image-prototype:latest

run:
	docker run -p 80:80 -it registry.yandex.net/junk/automl-image-prototype:latest

shell:
	docker exec -it `docker ps -q` /bin/bash

test:
	curl 127.0.0.1:80/v1/classify -X POST -F "data=@images/Female1.jpg" -F "model=badoo"
