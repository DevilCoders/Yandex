import httpx  # pip install httpx[http2]


client = httpx.Client(http2=True, http1=False)
#for url in ("https://www.google.com/robots.txt", "https://yandex.ru/robots.txt", "https://captcha-cloud-api-preprod.yandex.net/robots.txt", "https://antirobot-testing.yandex.net/robots.txt"):
#for url in ("https://www.google.com/robots.txt", "http://captcha-cloud-api-preprod.yandex.net/robots.txt", "https://antirobot-testing.yandex.net/robots.txt"):
#for url in ("http://antirobot-testing.yandex.net/robots.txt",):
#for url in ("http://captcha-cloud-api-preprod.yandex.net/yandex.cloud.priv.captcha.v1.CaptchaSettingsService/Create",):
#for url in ("http://captcha-cloud-api-preprod.yandex.net",):
for url in ("https://www.google.com/robots.txt", "http://antirobot-testing.yandex.net/robots.txt", "https://antirobot-testing.yandex.net/robots.txt"):
    print(f"\n\n{url}")
    response = client.get(url)
    print(f"{response.http_version} {response.status_code} {url}")
    print(response.headers)
    #print(response.text)
