# Building dataproc images with packer
For building packages u should install [packer](https://www.packer.io/).

## Examples:
Create new dataproc-image and nbs pool for compute-preprod:
`make dataproc-image`

Create image and nbs pool for compute-prod:
`make dataproc-image ENV=prod`

## Description
Dataproc image builds in two steps:
1. Build image with base packages: `make dataproc-base`
2. Build image with hadoop packages: `make dataproc-image`

Config `dataproc-base-1604.json` describes base dataproc image with ubuntu 16.04.
Config `dataproc-image-1604.json` describes additional steps executed on builded dataproc-base image.

Also, script `scripts/bootstrap-base-1604.sh` executed on first step, and `script/bootstrap-image-1604.sh` on second one.

After every modifition of packer configs, please run `make validate` to lint your changes.

Config files `variables-{preprod|prod}.json` used for parameterization of builds.
Packer configs also uses manifest post-processor for creating file with metainformation and saving imageId.
After that, `make` runs python script, that uses compute private-api and manifest for rewriting product id and creating nbs pooling cache.

Enjoy.
