# py-log-processor

Утилита для мониторинга организации мониторинга на основе tskv логов nginx.
Использует модуль file_read_backwards для эффективного чтения логов.
Запуск осуществляется с помощью
python3 log_processor.py settings.json, где settings.json - файл конфигурации.

Результат выдается в формате monrun в std::out.
Код возврата 0 - OK, 1 - Warning, 2 - Error.
Пример вывода:
2;CRITICAL:monitoring rate:rate: 0.16666666666666666;WARNING:monitoring percentile:100 percentile is 0.003;OK:monitoring count 400:;

Описание настроек:
```javascript
{
  "log_path": "example.log", # - путь до файла логов
  "depth_seconds": 120, - глубина вычитывания, начиная с последней строчки в логах
  "global_filter": { - глобальный фильтр
    "include": [ - regexp, то что входит в рассмортрение
      ".*"
    ],
    "exclude": [ - regexp, то что игнорируется
      ".*ping.*"
    ]
  },
  "log_monitor_settings": [ - настройки мониторинга, бывают 3 типов
    {
      "name": "monitoring count 400",
      "filter": {
        "include": [ - regexp, то что рассматривается в данном фильтре
          ".*400.*"
        ],
        "exclude": [] - regexp, то что не рассматривается в данном фильтре
      },
      "alert_configuration": {
        "type": "count", - тип алерта - количество записей в локальном фильтре
        "warning": 1,
        "critical": 2
      }
    },
    {
      "name": "monitoring percentile",
      "filter": {
        "include": [
          ".*"
        ],
        "exclude": []
      },
      "alert_configuration": {
        "type": "percentile", - перцентиль времен ответа в локальном фильтре
        "percentile": 100, - указывается в double, минимум 0 максимум 100
        "warning": 0.00001, - указывается в double, соответствует измерениям, принятым в анализируемом логе, чеще всего секунды
        "critical": 0.1
      }
    },
    {
      "name": "monitoring rate", - rate запией в локальном фильтре
      "filter": {
        "include": [
          ".*PUT.*"
        ],
        "exclude": []
      },
      "alert_configuration": {
        "type": "rate",
        "warning": 0.00001, - указывается в double, минимум 0, максимум 1
        "critical": 0.0002
      }
    }
  ]
}
```
