[Алерт vpc-control-micro-operation-errors в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvpc-control-micro-operation-errors)

## Что проверяет

Ошибки выполнения микроопераций. Горит **красным**, если за последние пять минут ошибок превысило 10. **Жёлтым**, если количество ошибок больше 0.

## Если загорелось

- смотреть логи сервиса `yc-vpc-control`, чтобы понять, с чем конкретно связаны ошибки
