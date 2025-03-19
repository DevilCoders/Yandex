.DEFAULT_GOAL = all

up:
	@mkdir -p ./logs
	docker-compose up --build --detach --force-recreate s3lib_nginx

down:
	docker-compose down

all: up
