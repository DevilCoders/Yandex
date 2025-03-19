Сборка пакетов
--------------

Пакет для бекенда собирается командой: ``ya package --debian --target-platform=linux --key=${DEV_KEY} ./yc-bootstrap-api/pkg.json``

Установка пакета
----------------

Пока что любым способом притаскиваем собранный пакет и дальше с помощью **dpkg -i ${package_name}** устанавливаем его. Наш api поднят как обычный systemd сервис, состояние можно увидеть командой **systemctl status yc-bootstrap-api.service**.
