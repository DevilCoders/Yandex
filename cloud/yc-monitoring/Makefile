i ?= main
e ?= preprod
m ?= dry-run
c ?= cfg
f =

.PHONY: all dashboards graphs alerts clusters services shards

all:
	yc-solomon-cli $(f) -c ./config.yaml -i $(i) -e $(e) -m $(m) update --tag $(c)

dashboards:
	yc-solomon-cli $(f) -c ./config.yaml -i $(i) -e $(e) -m $(m) update -o $(c)-dashboards

graphs:
	yc-solomon-cli $(f) -c ./config.yaml -i $(i) -e $(e) -m $(m) update -o $(c)-graphs

alerts:
	yc-solomon-cli $(f) -c ./config.yaml -i $(i) -e $(e) -m $(m) update -o $(c)-alerts

clusters:
	yc-solomon-cli $(f) -c ./config.yaml -i $(i) -e $(e) -m $(m) update -o $(c)-clusters

services:
	yc-solomon-cli $(f) -c ./config.yaml -i $(i) -e $(e) -m $(m) update -o $(c)-services

shards:
	yc-solomon-cli $(f) -c ./config.yaml -i $(i) -e $(e) -m $(m) update -o $(c)-shards
