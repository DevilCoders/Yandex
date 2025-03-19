# Overview

Generated code for the IAM Access Service v2 client in Python.

The sources can be found on [Bitbucket](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-proto-extensions).



# Publishing

To publish the artifact use the following command:

```bash
BUILD_NUMBER=XXX make -f python/Makefile clean upload-artifacts
```

Where `BUILD_NUMBER` is a part of the generated artifact version, in most cases should be provided by CI.

Twine is used to upload artifacts to PyPI. Auth credentials should be passed either via '~/.pypirc'
or environment variables (`TWINE_USERNAME` & `TWINE_PASSWORD`).
The access keys are available [here](https://pypi.yandex-team.ru/accounts/access-keys/).

The [docker container](https://teamcity.aw.cloud.yandex.net/buildConfiguration/IAM_DockerImages_IamFocalPython310BuildContainer)
makes it easier to run the publishing script. In case you don't use this container, make sure to install the
[required build dependencies](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-java/browse/iam/docker/iam-python-build-container/Dockerfile).
A simplified subset of the required dependencies can be installed with the command:
```bash
python3 -m pip install -r python/build_requirements.txt
```

The published module is available on [PyPI](https://pypi.yandex-team.ru/repo/default/yc-proto-extensions).
