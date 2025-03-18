Стрельбы по автосборке
====

```bash
cd java
ya make
./run.sh -j jdk/bin/java ru.yandex.ci.tools.FireDistbuild -h

Usage: <main class> [options]
  Options:
  * -a, --ammo
      File with build ids
  * -b, --build-per-hour

    -h, --help

  * -l, --launched
      Launched build ids file, will be created if not exists
    -n, --namespace
      DistBuild namespace
      Default: FR
  * -u, --useed-ammo
      Use build ids file (use for deduplication), will be created if not
      exists
```

Патроны (--ammo) - текстовый файл, где по ci-check-id на строку.
Например:
```
4g8dx
4g8dy
4g8dz
```

Пример запуска стрельб
```bash
./run.sh -j jdk/bin/java ru.yandex.ci.tools.FireDistbuild -a /Users/andreevdm/tmp/fire/ammo -u /Users/andreevdm/tmp/fire/used -l /Users/andreevdm/tmp/fire/lauched -b 500
```

