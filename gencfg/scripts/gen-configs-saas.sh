#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

run ./custom_generators/video_saas/generate_configs.py -d w-generated/all -T trunk
run ./custom_generators/video_saas/generate_configs.py -d w-generated/all -t video_refresh_dev -T trunk # REFRESH-179
run ./custom_generators/video_saas/generate_configs.py -d w-generated/all -t video_rt_dev_base -T trunk # GENCFG-973
run ./custom_generators/video_saas/generate_configs_images.py -d w-generated/all -T trunk
run ./custom_generators/video_saas/generate_configs_images_exp.py -d w-generated/all -T trunk # IMAGES-12562

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!!!!!!!!!!!!!!!!!!!! Everything is ok !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
