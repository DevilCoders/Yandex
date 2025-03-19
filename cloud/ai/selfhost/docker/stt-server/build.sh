#!/bin/bash

DOCKERFILE_DIR="$(dirname "$0")"
VERSION="latest"
PORT="17002"
MODEL="ru_dialog_general_e2e"
SERVER_MODE="YcOld"

if [[ ! -z "$1" ]]
then
    MODEL="$1"
fi

if [[ ! -z "$2" ]]
then
    VERSION="$2"
fi

if [[ ! -z "$3" ]]
then
    SERVER_MODE="$3"
fi

TAG=$(echo "$MODEL-$VERSION" | tr "_" "-")

# currently asr server take from https://sandbox.yandex-team.ru/resource/1161749327/view because it has some additional not trunk fixes
sky get -p rbtorrent:bcfa32f94d3c7586d3643c96be564bea250f4ccc # asr-server build, https://sandbox.yandex-team.ru/resources?type=VOICETECH_ASR_SERVER_GPU
sky get -p rbtorrent:84d30b5152bc5ebad6823a508ddee83c078e966d # normalizer, https://sandbox.yandex-team.ru/resources?type=VOICETECH_SERVER_REVERSE_NORMALIZER_DATA
if [[ "$MODEL" == "ru_dialog_general_e2e" ]]
then
    sky get -p rbtorrent:4c3d3b6caa63350aa42572c7819b2f46a4db0ce2 # ru_dialog_general_e2e, https://sandbox.yandex-team.ru/resources?type=VOICETECH_ASR_RU_RU_DIALOGENERALGPU

    # WARNING! the following commands applicable only to the specified language model and should probably be changed if you change LM's version
    sed -i '/BeamSearch/a\ \ \ "fix_final_word": false,' $MODEL/config.json
    sed -i '19,26d' $MODEL/config.json
fi

if [[ "$MODEL" == "cloud-lingware" ]]
then
    sky get -p rbtorrent:51a8dd38913b2e9fd763c100d03d9a5f05c59cc7 # cloud-lingware (Phone model), ask @f-minkin
fi

sed -e "s/{MODEL}/$MODEL/g" -e "s/{PORT}/$PORT/g" -e "s/{SERVER_MODE}/$SERVER_MODE/g" $DOCKERFILE_DIR/asr-server.tpl.xml > $DOCKERFILE_DIR/asr-server.xml

sudo docker build $DOCKERFILE_DIR --build-arg port=$PORT --build-arg model=$MODEL -t "cr.yandex/crppns4pq490jrka0sth/stt-server:$TAG"
sudo docker push "cr.yandex/crppns4pq490jrka0sth/stt-server:$TAG"
