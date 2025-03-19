=== WARNING ===

!!!
storage-secure.git is deprecated

Use yav.yandex-team.ru as a secrets storage

Here is the example of how you can extract secret from yav:
```yaml
  my_secret: {{ salt.yav.get('sec-01e3fgeh96axhgcc9wbn6q8zgt3[client_secret]') | json }}
```
!!!
