Собрать релиз: `releaser release_changelog`  
Сборка на TC: 


## oauth
Берет id и секрет приложения из settings.OAUTH_ID и settings.OAUTH_SECRET

```python
from django_ids.helpers import oauth

oauth.get_token_by_sessionid(sessionid='...')
oauth.get_token_by_uid(uid='12020202020')
oauth.get_token_by_password(username='login', password='secret')
```

## request.auth param
```
MIDDLEWARE_CLASSES = [
    'django_yauth.middleware.YandexAuthBackendMiddleware',
    ...
    'django_ids.auth.SetAuthMiddleware',
]
```
