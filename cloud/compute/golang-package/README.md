1. Скачать архив нужного пакета со страницы https://golang.org/dl/ ```wget https://dl.google.com/go/go1.14.3.linux-amd64.tar.gz```
2. Положить пакет архив в sandbox ```ya upload --ttl inf go1.14.3.linux-amd64.tar.gz``` (ttl --inf - чтобы сборка пакета была повторяемой в будущем)
3. Поправить в pkg.json версию golang и идентификатор ресурса, который будет записан в таске, которая создалась при загрузке исподника. (посмотреть можно в поле Resource with upload data)
4. Скопировать и запустить таску https://sandbox.yandex-team.ru/task/904719295/view
5. После завершения в разделе Logs (вверху главной страницы таски) перейти по ссылке log1, затем output.html, внизу будет написана версия пакета

```Creating debian package yc-compute-golang version 1.15.8-sandbox-task-904719295.arc-revision-7896709```
