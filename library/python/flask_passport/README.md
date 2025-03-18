# Flask Passport

Модуль для паспортной авторизации Flask

Пример использования в приложении:

    from flask import Flask, g
    from flask_passport import Authenticator

    app = Flask(__name__)
    Authenticator(app)

    @app.route("/")
    def hello():
        return "Hello {}!".format(g.identity.id)

Для передачи tvm тикета в blackbox необходимо явно задать PassportClient с необходимыми аргументами:

    Authenticator(app, passport=PassportClient(tvm_id=your_tvm_id, tvm_secret=your_tvm_secret))

# Что нужно сделать, чтобы всё заработало

- [Заказать](https://forms.yandex-team.ru/surveys/4901/) гранты blackbox
- [Почитать](https://wiki.yandex-team.ru/passport/corporate/) про требования внутреннего паспорта


# Установка github версии

    pip install -i https://pypi.yandex-team.ru/simple flask-passport
