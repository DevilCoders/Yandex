Библиотека доступна в артифактори http://artifactory.yandex.net/webapp/#/artifacts/browse/tree/General/yandex_common_releases/ru/yandex/yandex-annotations


Публикация
====



1. В `~/.m2/settings.xml` прописываем

```xml
<?xml version="1.0" encoding="UTF-8"?>
<settings xmlns="http://maven.apache.org/SETTINGS/1.0.0"
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
          xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0 http://maven.apache.org/xsd/settings-1.0.0.xsd">
    <servers>
        <server>
            <id>yandex-releases</id>
            <username>yandexcommondeploy</username>
            <password>yandexcommondeploy</password>
        </server>
    </servers>
</settings>
```

2. Выполняем (не забыв подставить _SVN_REVISION_HERE_ )
`ya make --maven-export --deploy --version=_SVN_REVISION_HERE_ --deploy --sources --repository-id yandex-releases --repository-url http://artifactory.yandex.net/artifactory/yandex_common_releases/`
