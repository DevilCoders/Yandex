### Clone repo with tts
```
mkdir contrib
git -C contrib clone https://bb.yandex-team.ru/scm/~d-kruchinin/tts.git --branch ASREXP-147-variables-synthesis
```

### generate and put here proto stubs
```
python3 -m grpc_tools.protoc -I . -I $GOPATH/src/bb.yandex-team.ru/cloud/cloud-go/public-api/third_party/googleapis --python_out=./ --grpc_python_out=./ $GOPATH/src/bb.yandex-team.ru/cloud/cloud-go/public-api/yandex/cloud/ai/tts/v1/tts_service.proto $GOPATH/src/bb.yandex-team.ru/cloud/cloud-go/public-api/yandex/cloud/validation.proto
```

### download dependencies and build tt-preprocessor-cli put them into paths ./vocoder ./lingware ./tacotron ./preprocessor
Details: https://wiki.yandex-team.ru/users/d-kruchinin/tacotron-instrukcija-po-sintezu/

