from clan_tools.vh.porto import build_nirvana_layer

import logging.config
from clan_tools.utils.conf import read_conf
config = read_conf('config/logger.yml')
logging.config.dictConfig(config)


env_script = '''
apt update --allow-insecure-repositories --allow-unauthenticated
apt install --yes --force-yes \\
  python3.7-dev python3.7-venv python3-venv python3-pip \\
  libopenmpi-dev openmpi-bin cython3 build-essential g++ 
  
python3.7 -m venv /opt/python

/opt/python/bin/pip install requests==2.23.0
/opt/python/bin/pip install -i https://pypi.yandex-team.ru/simple/ \\
 yandex-tracker-client==1.7 \\
 yandex-yt==0.9.9



# cache is already disabled
/opt/python/bin/pip install \\
  tsfresh==0.15.1 \\
  mlflow==1.7.2 \\
  imbalanced-learn==0.6.2 \\
  xgboost==1.0.2 \\
  zeep==3.4.0 \\
  xmltodict==0.12.0 \\
  "dask[complete]"==2.13.0 \\
  graphviz==0.13.2 \\
  pydot==1.4.1 \\
  numba==0.48.0 \\
  catboost==0.22 \\
  pandas==1.0.3 \\
  matplotlib==3.2.1 \\
  scikit-learn==0.22.2 
'''




if __name__ == '__main__':
    build_nirvana_layer(
        environment_script=env_script,
        sandbox_token='bakuteev-sandbox-token',
        layer_name='py37analytics',
        quota = 'external-activities')
   
