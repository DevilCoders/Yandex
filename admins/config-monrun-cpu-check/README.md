Если нужно переопределить пороговые значения critical и warning, то надо создать файл /etc/config-monrun-cpu-check/config.yml с примерно таким содержанием:
warning: 50
critical: 60

Для сборки нужно чтобы в системе правильно работал Golang (путь до утилиты go и переменная окружения GOPATH)
Команда для сборки:
debuild --preserve-envvar=PATH --preserve-envvar=GOPATH
