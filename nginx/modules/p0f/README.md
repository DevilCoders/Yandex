p0f - фингерпринтинг клиентов по свойствам TCP/IP соединения (https://en.wikipedia.org/wiki/P0f)

# Директивы
### http
```
http {
  p0f (on|off);
  p0f_map_size (\d+, default = 10000);
  ...
}
```

# Переменные
```
$p0f_fingerprint  // "6:49+15:0:1300:65535,8:mss,sok,ts,nop,ws:flow:0"
```

# Пример использования в конфигурации

```
http {
    server_tokens off;
    charset       utf-8;

    access_log    logs/access.log  combined;
    p0f on;
    p0f_map_size 10000;
    
    server {
        server_name   localhost;
        listen 8085 default;
        listen [::]:8085 default;

        error_page    500 502 503 504  /50x.html;

        location      / {
            return 200 "$p0f_fingerprint";
            add_header Content-Type text/plain always;
        }

    }
}
```
