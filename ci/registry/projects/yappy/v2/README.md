# Тасклеты для создания и управления Yappy бетой

По умолчанию все тасклеты запускаются
на [Multislot](https://wiki.yandex-team.ru/sandbox/clients/#client-tags-multislot) хостах.

## projects/yappy/v2/create_beta

Тасклет позволяет создать Yappy бету по заданным параметрам. Выставляет по
дефолту `bolver_spec.use_meta_cgi=True`

```yaml
create-beta:
    title: Create Yappy beta
    task: projects/yappy/v2/create_beta
    input:
        # Параметры для именования беты, можно указать только один
        # Если будет указан name, то это будет именем беты
        # Если будет указан slug, то имя беты будет сформировано
        # как slug + "-${pull_request_id}", что удобно для бет в pr
        beta_config:
            name: ydo-request-extractor-pr
            slug: ydo-request-extractor-pr
        # Параметры использования yappy
        yappy_config:
            base_url: yappy.z.yandex-team.ru # (optional)
            token:
                uuid: sec-01eejahmvyzqjv50qt9smmq898 # sec или ver секрета в vault
                key: YAPPY_TOKEN # ключ, по которому лежит Oauth токен
        # Параметры авторизации
        # Кому выдавать права на созданные объекты
        auth_config:
            logins: # Список staff логинов. Автоматически расширяется автором ревью
                - zhshishkin
            abc_services: # Список id abc сервисов
                - 2776
        # Объект ApiBeta, на основе которого и создастся бета
        # https://a.yandex-team.ru/arc_vcs/search/priemka/yappy/proto/structures/api.proto?rev=r9028163#L42
        base_beta: { }
```

Если бета существует, то ничего не произойдет

## projects/yappy/v2/add_component_to_beta

Тасклет позволяет добавить в Yappy бету компоненту. Может также предварительно создать тип
компоненты и квоту

```yaml
add-component-to-beta:
    title: Add component to beta
    task: projects/yappy/v2/add_component_to_beta
    input:
        # Если используется в связке с create_beta, то следующие 3 конфига можно не указывать
        beta_config: ...
        yappy_config: ...
        auth_config: ...

        # Объект ApiComponentType, на основе которого и создастся тип
        # https://a.yandex-team.ru/arc_vcs/search/priemka/yappy/proto/structures/api.proto?rev=r9028163#L114
        # К name будет добавлена строка "-${pull_request_id}"
        component_type_to_create: { }

        # Объект ApiQuota, на основе которого и создастся квота
        # https://a.yandex-team.ru/arc_vcs/search/priemka/yappy/proto/structures/api.proto?rev=r9028163#L133
        # К name будет добавлена строка "-${pull_request_id}"
        quota_to_create: { }

        # Объект ApiBetaComponent, база для компонента добавляемого в бету
        # https://a.yandex-team.ru/arc_vcs/search/priemka/yappy/proto/structures/api.proto?rev=r9028163#L72
        base_beta_component:
            # если не указать, то будет создан и использован тип из component_type_to_create
            type: ydo-request-extractor
            # если не указать, то будет создана и использована квота из quota_to_create
            quota: ydo-request-extractor

```

Если у `ApiBetaComponent` не указаны `checks`, то будут выставлены дефолтные за исключением проверки
на наличие `instancectl.conf`

## projects/yappy/v2/apply_patch

Тасклет позволяет добавить патч на файлы в компоненту Yappy беты

```yaml
apply-patch:
    title: Apply patch to beta component
    task: projects/yappy/v2/apply_patch
    input:
        # Если используется в связке с create_beta и add_component_to_beta,
        # то следующие 3 конфига можно не указывать
        beta_config: ...
        yappy_config: ...
        component_config: ...

        patch:
            # какой тип ресурса взять из контекста для подстановки в патч
            -   path: extract_backend_request_source
                sandbox_type: YDO_BACKEND_REQUEST_EXTRACTOR

                # будет использоваться файл со статическим содержимым
            -   path: static_file
                content: "zerodiff"

                # файл удалится из сервиса
            -   path: file_to_delete
                delete: true

                # обновления в базовом сервисе не будут подтягиваться в бету
            -   path: file_without_update
                no_manage: true


```

## projects/yappy/v2/wait_for_consistency

Тасклет позволяет дождаться консистентности беты

```yaml
wait-for-consistency:
    title: Wait for beta consistency
    task: projects/yappy/v2/wait_for_consistency
    input:
        # Если используется в связке с create_beta, то следующие 2 конфига можно не указывать
        beta_config: ...
        yappy_config: ...
```

## projects/yappy/v2/post_beta_links

Тасклет позволяет дождаться консистентности беты

```yaml
post-beta-links:
    title: Add links to pr
    task: projects/yappy/v2/post_beta_links
    input:
        # Если используется в связке с create_beta и add_component_to_beta,
        # то следующие 3 конфига можно не указывать
        beta_config: ...
        yappy_config: ...
        component_config: ...

        arcanum_config:
            token:
                uuid: sec-01eejahmvyzqjv50qt9smmq898 # sec или ver секрета в vault
                key: ARCANUM_TOKEN # ключ, по которому лежит Oauth токен

        links_config:
            url_path: uslugi # путь, который будет добавлен в ссылку для попадания в бету
```

## projects/yappy/v2/deallocate_beta

Тасклет позволяет деаллоцировать бету, чтобы освободить занимаемый ею слот. Обычно используется в
`cleanup-jobs`

```yaml
cleanup-yappy:
    title: Deallocate beta
    task: projects/yappy/v2/deallocate_beta
    input:
        # необходимо указать как в create_beta, чтобы объект нашелся в Yappy
        beta_config: ...
        yappy_config: ...
```

# Пример a.yaml

Пример можно
посмотреть [тут](https://a.yandex-team.ru/arc_vcs/search/priemka/yappy/tasklets/v2/example/a.yaml?rev=null#L32)
