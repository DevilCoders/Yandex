release:
	ya package ./pkg.json --docker --docker-repository=cdc --docker-network=host --docker-push

run_server:
	ya make server && ./server/server --prefix="$PROFILER_PREFIX"

run_ui:
	cd ui && npm run dev
