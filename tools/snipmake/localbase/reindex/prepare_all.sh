#!/usr/local/bin/bash

# Подробности по работе всего этого можно посмотреть здесь:
# http://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/SnipML/LocalBaseOperations/reindex

# Путь к нужному дереву файлов, необходимым для работы prewalrus и т.д.
# Действительно только для scrooge
USEFUL_FILES_PATH="/place/home/alivshits/SHARED/reindex_useful_files"

# Путь к аркадии, из которой будут собраны все необходимые бинарники
# Тут должен быть путь к аркадии из роботного стейбла
ARCADIA_PATH="../../../../../arcadia"

# Папка в которой будет проходить переиндексация
REINDEX_DIR_PATH="reindex"

# Директория, в которую будут сохранены симлинки на нужные бинарники
# Её менять не надо, все бинарники должны лежать в корне папки, где
# будет проходить переиндексация
BIN_DIR=$REINDEX_DIR_PATH

# Путь к базе, которую нужно переиндексировать
BASE_PATH="/place/home/nosyrev/index/dec_useful_stable_final"

# Создание нужной файловой структуры
python prepare_dir_tree.py -s $USEFUL_FILES_PATH -d $REINDEX_DIR_PATH
# Копирование нужных бинарников
#./collect_binaries.sh $ARCADIA_PATH $BIN_DIR
# Создание ссылок на файлы из переиндексируемой базы
python prepare_index_links.py $BASE_PATH $REINDEX_DIR_PATH
# Копирование нужных скриптов
cp prewalrus.sh $REINDEX_DIR_PATH/prewalrus.sh
cp mergearchive.sh $REINDEX_DIR_PATH/mergearchive.sh
cp readme.txt $REINDEX_DIR_PATH/readme.txt

