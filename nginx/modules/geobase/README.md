geobase - аналог geoip(http://nginx.org/ru/docs/http/ngx_http_geoip_module.html) использующий библиотеку geobase

# Директивы
```
  geobase файл;
```
```
  geobase_proxy адрес | CIDR;
```
```
  geobase_proxy_recursive on | off;
```

# Новые переменные
* $geobase_country = 0        // "RU"
* $geobase_country_name       // "Russia"
* $geobase_country_part       // "Central"
* $geobase_country_part_name  // "Central Federal District"
* $geobase_region             // "RU-MOS"
* $geobase_region_name        // "Moscow and Moscow Oblast"
* $geobase_city               // "RU MOW"
* $geobase_city_name          // "Moscow"
* $geobase_latitude
* $geobase_longitude
* $geobase_path               // "RU/Central/RU-MOS/RU MOW"
* $geobase_path_name          // "Russia/Central Federal District/...."
