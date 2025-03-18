.PHONY: ALL

build-release:
	@ya package package.json -r --target-platform=linux --docker --docker-repository=go

publish-release:
	@ya package package.json -r --target-platform=linux --docker --docker-repository=go --docker-push

build-latest:
	@ya package package.json -d --custom-version=latest --target-platform=linux --docker --docker-repository=go

publish-latest:
	@ya package package.json -d --custom-version=latest --target-platform=linux --docker --docker-repository=go --docker-push