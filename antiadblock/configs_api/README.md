## Админка антиадблока ака antiadblock-configs-api

* [Документация на  API](docs/API.md)
* [Как разрабатывать](docs/Development.md)
* [Как тестировать](docs/Testing.md)
* [Как релизить](docs/Releasing.md)
* [Про права](docs/Auth.md)

## Environments

### Production
#### backend 
url: https://api.aabadmin.yandex.ru/   
nanny-namespace (L7-балансер): https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/api.aabadmin.yandex.ru/show/  
L3-балансер: https://l3.tt.yandex-team.ru/service/7105  
samogon deployment: https://aabadmin.n.yandex-team.ru

#### frontend
url: https://antiblock.yandex.ru  
deploy: https://deploy.yandex-team.ru/stage/antiadb-front-production

### Preprod
#### backend 
url: https://preprod.aabadmin.yandex.ru/  
nanny-namespace (L7-балансер): https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/preprod.aabadmin.yandex.ru/show/  
L3-балансер: https://l3.tt.yandex-team.ru/service/7090  
samogon deployment: https://aabadmin11.n.yandex-team.ru

#### frontend
url: https://preprod.antiblock.yandex.ru/  
deploy: https://deploy.yandex-team.ru/stage/antiadb-front-preprod

### Testing (for developers)
#### backend 
url: https://test.aabadmin.yandex.ru/  
nanny-namespace (L7-балансер): https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/test.aabadmin.yandex.ru/show/  
L3-балансер: https://l3.tt.yandex-team.ru/service/6895  
samogon deployment: https://aabadmin23.n.yandex-team.ru (или другая тестовая инсталляция)

#### frontend
В качестве фронта можно использовать можно использовать develop, но необходимо изменить значение переменной окружения BACKEND_API_URL
url: https://develop.antiblock.yandex.ru/  
deploy: https://deploy.yandex-team.ru/stage/antiadb-front-develop
