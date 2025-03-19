#MDB Maintenance

MDB Maintenance включает в себя:
* `configs` - набор конфигов, которыми описываются задачи на обслуживание кластеров
* `service` - сервис, который отвечает за постановку задач на конкретный набор кластеров

### Design
https://miro.com/app/board/o9J_kuS9Tr0=/
![Maintenance Task Design](https://jing.yandex-team.ru/files/frisbeeman/mw-design-v01.jpg)

### Config sections
#### ID
`$ uuidgen | tr "[:upper:]" "[:lower:]"`

#### Cluster selection
Section query have to return list of `cid` to plan maintenance.

#### Pillar changes
Section need to modify a pillar by UPDATE query. In the query you have to use `:cid` to restrict changes by only one cluster.
RETURNING * of result of the query will be used to control the changes.

#### Worker task
`type` and `args` is `dbaas_worker`task parameters.
