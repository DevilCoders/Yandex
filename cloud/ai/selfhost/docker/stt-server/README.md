## Release

1. Add new record in changelog

2. Go to one of machines that have access to skynet

3. Login in container registry using one of methods https://cloud.yandex.ru/docs/container-registry/operations/authentication

4. Run:
```bash
cd cloud/ai/selfhost/docker/stt-server
export VERSION=<next_version_tag>
sudo bash build.sh ru_dialog_general_e2e ${VERSION}

# After updating changelog
ya pr create -m "Releasing cloud ai stt-server:${VERSION} image"
```

TODO: Add tagging, automate
