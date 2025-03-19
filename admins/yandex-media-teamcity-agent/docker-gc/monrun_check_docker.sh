#!/bin/bash

DOCKER_BIN=$(which docker)
if [ -z "${DOCKER_BIN}" ]; then
    echo "2;Docker binary not found"
    exit
fi

docker_version=$(${DOCKER_BIN} version --format "{{.Server.Version}}" \
    2>/dev/null)
ret_code=$?
if [ ${ret_code} -ne 0 ]; then
    echo "2;Failed to connect to Docker (server is not running or"\
        "not enough permissions)"
    exit
fi

# Any future checks for ${docker_version}
if [ -z "${docker_version}" ]; then
    echo "2;Empty Docker version"
    exit
fi

# Keep brackets (-3): otherwise it will be treated as ${parameter:-word}
if [ ${docker_version:(-3)} != "-ce" ]; then
    echo "2;Non -ce version of Docker"
    exit
fi

echo "0;Ok"
exit 0
