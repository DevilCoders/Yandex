### FileWithHeader - библиотека для сохранения бинарных файлов вместе с человеко-читаемым описанием

Создаваемые библиотекой файл состоит из дух частей: header и body.

header - это текстовые данные, который можно прочитать через

```(bash)
head my_file.bin.htxt
```

body - это произвольные бинарные данные.

Свойства:
1. у body есть гарантия на выравнивание первого байта
1. имеет выделенные поля для версионирования (есть версия либы, и версия, которую задаёт клиент библиотеки)
1. в header есть поле для произвольных данных (например туда можно положить json)
1. при правильном использовании по файлу можно понять каким кодом оно варилось
1. файлы тегируются, предотвращая отдачу заведомо неправильного файла на парсинг (пути перепутались например).


Примеры отчётов
```
> head cogips_dict.trie.htxt
CogiponimsDict  2.0.0
Comment this is file with cogiponims classification, see SEARCH-4411
HeaderVer       2.0.0
BuildTime       2018-05-24T14:17:16.254788+0300
BuildInfo       Svn info:\n    URL: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia\n    Last Changed Rev: 3638071\n    Last Changed Author: superman\n    Last Changed Date: 2018-05-22 12:22:10 +0300 (Tue, 22 May 2018)\n\nOther info:\n    Build by: ilnurkh\n    Top src dir: /place/home/ilnurkh/allex/arc3\n    Top build dir: /place/home/ilnurkh/.ya/build/build_root/faif/000029\n    Hostname: factordev3.search.yandex.net\n    Host information: \n        Linux factordev3.search.yandex.net 4.4.0-53-generic #74-Ubuntu SMP Fri Dec 2 15:59:10 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux\n
CppBuilderLocation      quality/factors/lingboost/allex/tools/make_cogiponims_dicts_from_tsv/main.cpp:205



LastLine, perform alignmentskip -------------------------------
```


```
> head hnsw_graph.dat.htxt -n 11
SimpleLayeredGraph      1.0.0
Comment layered hnsw graph, each layer is regular
HeaderVer       2.0.0
BuildTime       2018-10-26T10:19:23.919433+0300
BuildInfo       Svn info:\n    URL: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia\n    Last Changed Rev: 4132109\n    Last Changed Author: tervlad\n    Last Changed Date: 2018-10-25 09:43:31 +0300 (Thu, 25 Oct 2018)\n\nOther info:\n    Build by: ilnurkh\n    Top src dir: /home/ilnurkh/knn\n    Top build dir: /home/ilnurkh/.ya/build/build_root/ihr5/000811\n    Hostname: ilnurkhdev.vla.yp-c.yandex.net\n    Host information: \n        Linux ilnurkhdev.vla.yp-c.yandex.net 4.4.114-50 #1 SMP Fri Feb 2 10:53:13 UTC 2018 x86_64 x86_64 x86_64 GNU/Linux\n
CppBuilderLocation      quality/relev_tools/knn/lib/hnsw_graph/simple_layered_graph_impl.cpp:81
CustomHeader    {\"NumVertexesAtLevel\":[39742371,4967796,620974,77621,9702,1212,151,18,2],\"NumNeighborsInLevels\":[16,16,16,16,16,16,16,16,1]}



LastLine, perform alignmentskip --------
```
