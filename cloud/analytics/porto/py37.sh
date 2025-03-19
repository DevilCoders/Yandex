#!/bin/bash
set -ex


cat <<EOT > /etc/apt/sources.list.d/yandex-taxi-xenial.list
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial prestable/all/
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial prestable/amd64/
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial stable/all/
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial stable/amd64/
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial testing/all/
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial testing/amd64/
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial unstable/all/
deb http://yandex-taxi-xenial.dist.yandex.ru/yandex-taxi-xenial unstable/amd64/
EOT


apt-get update
apt-get install -y python3.7-dev python3-pip
python3.7 -m pip install --upgrade pip


python3.7 -m pip install --no-cache-dir -i https://pypi.yandex-team.ru/simple/ \
  tsfresh==0.17.0 \
  imbalanced-learn==0.7.0 \
  xgboost==1.4.1 \
  zeep==4.1.0 \
  xmltodict==0.12.0 \
  graphviz==0.16 \
  pydot==1.4.2 \
  numba==0.55.1 \
  catboost==1.0.4 \
  category-encoders==2.2.2 \
  scikit-learn==1.0.2 \
  simplejson==3.17.6 \
  tabulate==0.8.9 \
  umap-learn==0.5.1 \
  regex==2022.3.15 \
  ujson==5.1.0 \
  pymorphy2==0.9.1 \
  gensim==4.1.2 \
  nltk==3.7 \
  orjson==3.6.7  \
  graphframes==0.6 \
  networkx==2.6.3 \
  pytz==2022.1


python3.7 -m pip install   --no-cache-dir -i https://pypi.yandex-team.ru/simple/ \
 startrek-client==2.5 \
 yandex-passport-vault-client==1.5.2 \
 yandex-tracker-client==2.2 \
 yandex-yt-yson-bindings==0.4.0.post0


python3.7 -m pip install  --no-cache-dir -i https://pypi.yandex-team.ru/simple \
   yandex-yt==0.11.1 \
   cyson==1.17.1 \
   scipy==1.7.3

python3.7 -m pip install  --no-cache-dir -i https://pypi.yandex-team.ru/simple \
   pandas==1.3.5 \
   pyarrow==6.0.1

python3.7 -m pip install  --no-cache-dir -i https://pypi.yandex-team.ru/simple \
   python-Levenshtein==0.12.2

python3.7 -m nltk.downloader stopwords

mkdir -p /opt/python3.7/bin
ln -s /usr/bin/python3.7 /opt/python3.7/bin/python
ln -s /usr/bin/python3.7 /opt/python3.7/bin/python3.7
