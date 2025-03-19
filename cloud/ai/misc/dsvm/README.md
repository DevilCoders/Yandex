# Data Science Virtual Machine

### Введение

Yandex Data Science Virtual Machine (далее — DSVM) — это виртуальная машина с предустановленными популярными платформами для машинного обучения, такими как CatBoost, LightGBM, XGBoost, TensorFlow, PyTorch и Keras. DSVM предоставляет специалистам по машинному обучению и ученым среду для обучения и экспериментов с современными моделями на данных.

### Спецификация
  * Окружения conda c Python 2 и Python 3
  * Jupyter Notebook/Lab
  * ML пакеты
    * catboost
    * xgboost
    * lightgbm
    * tensorflow
    * pytorch
    * keras
  * Компиляторы
    * clang
    * gcc
    * nodejs
    * go

### Начало работы
  1. Заходим по ssh
  2. Видим welcome message
  3. Выбираем нужное окружение питона (2 или 3)
  4. Дальше, например, https://tech.yandex.com/catboost/doc/dg/concepts/python-quickstart-docpage/


### Ссылки
  * CatBoost - https://catboost.yandex/
  * Xgboost - https://xgboost.readthedocs.io/en/latest/
  * LightGbm - https://github.com/Microsoft/LightGBM
  * TensorFlow - https://www.tensorflow.org/
  * pytorch - https://pytorch.org/
  * Keras - https://keras.io/
  
### Docker
  Собрать: ```docker build -t dsvm_image .```
  Запустить: ```docker run -i -t dsvm_image /bin/bash -l```
