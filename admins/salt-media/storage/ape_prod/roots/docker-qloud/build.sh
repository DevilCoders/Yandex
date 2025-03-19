docker build . -t registry.yandex.net/cocaine-layers2image-`head -n 1 02-app_bin_layer.sh | cut -d= -f2`:`date +"%Y%m%d-%H%M%S"` --no-cache --network=host

