from flask import Flask, request, render_template, flash, redirect, url_for
import os
import sys
import json
from flask_wtf import FlaskForm
from wtforms import TextField, SubmitField, DecimalField, SelectField, BooleanField, HiddenField, validators
from io import BytesIO
import base64
import requests
import argparse


SECRET_KEY = os.urandom(32)
SMARTCAPTCHA_SERVER_KEYS = None
args = None


root_path = os.getenv("ROOT_PATH", os.getcwd())
static_folder = os.getenv("STATIC_FOLDER", os.path.join(root_path, './templates'))
print(f"root_path = {root_path}", file=sys.stderr)
print(f"static_folder = {static_folder}", file=sys.stderr)
app = Flask(__name__, root_path=root_path, static_url_path='', static_folder=static_folder, template_folder=static_folder)
app.config['SECRET_KEY'] = SECRET_KEY


class MainForm(FlaskForm):
    name = TextField("Name", default="user", validators=[validators.Length(min=2, max=20)])
    #spravka = HiddenField("Spravka", default="")
    submit = SubmitField("Send")


def check_captcha(token, template_idx):
    resp = requests.get(
        f"{args.captcha_api_host}/validate",
        {
            "secret": SMARTCAPTCHA_SERVER_KEYS[template_idx - 1],
            "token": token,
            "ip": request.headers.get("X-Forwarded-For-Y", "127.0.0.1")  # Нужно передать IP пользователя.
                                                                         # Как правильно получить IP зависит от вашего прокси.
        },
        timeout=1
    )
    server_output = resp.content.decode()
    if resp.status_code != 200:
        print(f"Allow access due to an error: code={resp.status_code}; message={server_output}", file=sys.stderr)
        return True
    return json.loads(server_output)["status"] == "ok"


@app.route('/', methods=['GET'])
def index():
    return redirect(url_for('demo1'))

def common_demo(idx):
    form = MainForm()
    template = f"demo{idx}.html"
    kwargs = {
        'form': form,
        'captcha_api_host': args.captcha_api_host,
    }
    if request.method == 'POST':
        if form.validate() == False:
            flash('All fields are required.')
            return render_template(template, **kwargs)
        else:
            if not check_captcha(request.form["smart-token"], idx):
                return render_template(template, error="Captcha validation failed", **kwargs)

            name = request.form["name"]
            return render_template(template, greeting=f"Hello, {name}!", **kwargs)

    elif request.method == 'GET':
        return render_template(template, **kwargs)

@app.route('/captcha-demo-1', methods=['GET', 'POST'])
def demo1():
    return common_demo(1)

@app.route('/captcha-demo-2', methods=['GET', 'POST'])
def demo2():
    return common_demo(2)

@app.route('/captcha-demo-3', methods=['GET', 'POST'])
def demo3():
    return common_demo(3)

@app.route('/captcha-demo-4', methods=['GET', 'POST'])
def demo4():
    return common_demo(4)

@app.route('/captcha-demo-5', methods=['GET', 'POST'])
def demo5():
    return common_demo(5)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=5000, type=int, help="Port")
    parser.add_argument("--captcha-api-host", default='https://captcha-test.yandex-team.ru', help="Captcha API Host")
    parser.add_argument("--keys", required=True, help="Comma separated server keys")
    return parser.parse_args()

def main():
    global args, SMARTCAPTCHA_SERVER_KEYS
    args = parse_args()
    SMARTCAPTCHA_SERVER_KEYS = args.keys.split(',')
    app.run(debug=False, host='::', port=args.port)


if __name__ == '__main__':
    main()
