#!/bin/bash
app="docker.test"
docker build -t ${app} .
docker run -d -p 8000:8000 \
  --name=${app} \
  ${app}
