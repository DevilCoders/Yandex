[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dnvidia-vgpu-mgr)

## nvidia-vgpu-mgr
Проверяет, что systemd-сервис `nvidia-vgpu-mgr` запущен и работает с точки зрения systemd.

Присутствует работает на машинах с vGPU.
Проверяет, что сервис `yc-gpu-servicevm` работает и может принимать запросы
(то есть здоров, `healthy`).

## Подробности
`nvidia-vgpu-mgr` это один из компонентов NVIDIA vGRID, необходимый для его работы.

Нужен в момент запуска виртулки с vGPU.

Из наблюдаемого поведения, кажется этот сервис при запуске первой виртуалки с vGPU на конкретной карте
запускает на ней процесс `vgpu`, который можно увидеть в выводе `nvidia-smi`.

Именно этот процесс `vgpu` важен для работоспособности vGPU карт внутри виртуалок.
При смерти или перезапуске `nvidia-vgpu-mgr` эти процессы `vgpu` не трогаются.
При смерти процессов `vgpu` сервис `nvidia-vgpu-mgr` никак на это не реагирует.
Сервис, похоже, никак не реагирует на смерть этих `vgpu` процессов.

## Диагностика и recovery
Если сервис упал, то запустить его.
Желательно попробовать понять почему он упал, посмотрев в логи `journalctl -u nvidia-vgpu-mgr`.

Посмотреть, что с картами можно при помощи `nvidia-smi`.

Если у пользователей есть какие-то проблемы с vgpu внутри виртуалки, то кажется всё что можно - попробовать перезапустить её
из консоли/api облака. Перезапуск изнутри виртуалки не поможет. Пример проблемы:
```
xelez@xelez-a100-vgpu-mig-test:~/NVIDIA_CUDA-11.4_Samples/4_Finance/MonteCarloMultiGPU$ nvidia-smi
Unable to determine the device handle for GPU 0000:8B:00.0: Unknown Error
```


## Официальная документация
Есть официальный гайд по troubleshooting'у vgpu:
https://docs.nvidia.com/grid/latest/grid-vgpu-user-guide/index.html#troubleshooting-grid-vgpu

Но из полезного там, кажется, только совет посмотреть в логи.
