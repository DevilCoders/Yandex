# image-component
прототип cлужбы управления образами машин


# Генерирование protobuf
protobuf-файлы генерируютя для каждой системы (python, swagger, go) из файла snapshot_raw.proto с помощью C-препроцессора 

## Для генерирования библиотек на языках требуется установить внешние зависимости  
apt install protobuf-compiler libprotobuf-dev
pip install grpcio-tools

go/bin должен быть в PATH - туда компилируются утилиты, необходимые для сборки go-кода

# Для тестов 
* наличие qemu-img в PATH
* linux-варианты утилит командной строки, например readlink. На MacOS можно взять в brew install coreutils, затем PATH="/usr/local/opt/coreutils/libexec/gnubin:$PATH"
