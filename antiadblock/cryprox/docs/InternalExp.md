## Эксперименты во внутренней сети

<b>Эксперимент запустится при выполнении двух условий:</b>
1. Создан SB-ресурс с конфигом для экспериментов
2. В админке включен эксперимент в конфиге соответтвующего сервиса
### Создание SB-ресурса с конфигом для внутренних экспериментов
1. Воспользоваться таской `CREATE_TEXT_RESOURCE`  
Для этого заходим в [Sandbox](https://sandbox.yandex-team.ru) и создаем таску указанного типа   
![](https://jing.yandex-team.ru/files/dridgerve/internal_exp_3.png)    
![](https://jing.yandex-team.ru/files/dridgerve/internal_exp_4.png)    
1. Заполнить необходимые поля в таске   
Обязательно указать `Owner: ANTIADBLOCK`    
![](https://jing.yandex-team.ru/files/dridgerve/internal_exp_1.png)    
В полях `Resource type` и `Created resource name` указать `ANTIADBLOCK_CRYPROX_INTERNAL_EXPERIMENT_CONFIG`    
В `Resource file content` положить json с конфигом в формате:
```json
{"service_id_1":"exp_id_1","service_id_2":"exp_id_2"}
``` 
Указываем `ttl` в днях через аттрибуты (или включаем бесконечноге хранение)   
![](https://jing.yandex-team.ru/files/dridgerve/internal_exp_2.png)    
1. Для выключения всех экспериментов содержимое должно буть пустым json'ом ```{}```  
1. [Список ресурсов с конфигами для экспериментов](https://sandbox.yandex-team.ru/resources?type=ANTIADBLOCK_CRYPROX_INTERNAL_EXPERIMENT_CONFIG&limit=20)

### Включение экспериментального конфига в админке
1. Заходим в нужный сервис
1. Выбираем нужный конфиг (не активный и не тестовый) и запускаем эксперимент. Название экспа должно совпадать с названием в конфиге, созданным выше   
1. Для окончания эксперимента останавливаем его в админке
