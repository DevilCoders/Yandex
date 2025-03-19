
for-linux-in-docker:
	docker build . --tag pg-build
	docker run  --volume pg:/recipe/ pg-build

clean-docker:
	docker container prune -f
	docker volume prune -f

