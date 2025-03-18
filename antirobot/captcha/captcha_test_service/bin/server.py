from flask import Flask, request, render_template, flash
import os
import sys
import json
from numpy import pi
from flask_wtf import FlaskForm
from wtforms import TextField, SubmitField, DecimalField, SelectField, BooleanField, HiddenField, validators
from antirobot.captcha.captcha_test_service.bin import logic
from io import BytesIO
import base64
import requests
import argparse
import ticket_parser2.api.v1 as tp2


SECRET_KEY = os.urandom(32)
tvm_client = None
args = None


def generate(text, curvature, color_shift, noise, background, font, squeeze, arc, reversed_arc, aspect_ratio, mode,
             shade_val):
    cr = logic.CaptchaRenderer(font, float(curvature), float(color_shift), float(noise), float(aspect_ratio),
                               float(squeeze), mode, float(shade_val))
    return cr.generate_image(text, background, float(arc) * pi, bool(reversed_arc))


root_path = os.getenv("ROOT_PATH", os.getcwd())
static_folder = os.getenv("STATIC_FOLDER", os.path.join(root_path, '../app/build'))
print(f"root_path = {root_path}", file=sys.stderr)
print(f"static_folder = {static_folder}", file=sys.stderr)
app = Flask(__name__, root_path=root_path, static_url_path='', static_folder=static_folder,
            template_folder=static_folder)
app.config['SECRET_KEY'] = SECRET_KEY


def serve_pil_image(pil_img):
    img_io = BytesIO()
    pil_img = pil_img.convert("L").convert("RGB")
    pil_img.save(img_io, 'JPEG', quality=100)
    img_io.seek(0)
    return img_io.read()


class MainForm(FlaskForm):
    name = TextField("Text", default="helloworld", validators=[validators.Length(min=2, max=20)])
    curvature = DecimalField("curvature", default=0.0)
    color_shift = DecimalField("Min color shift", default=0.3)
    noise = DecimalField("Noise", default=0.2)
    squeeze = DecimalField("Squeeze", default=1.0)
    aspect_ratio = DecimalField("Aspect ratio", default=0.33)
    mode_choices = [(logic.CM_CONTRAST, logic.CM_CONTRAST), (logic.CM_NONE, logic.CM_NONE),
                    (logic.CM_IMPAINTING, logic.CM_IMPAINTING)]
    mode = SelectField("Mode", choices=mode_choices, default=logic.CM_CONTRAST)
    shade_val = DecimalField("Shade", default=0.3)
    arc = DecimalField("Arc (* π)", default=1.0)
    backgrounds = os.listdir("backgrounds") if os.path.exists("backgrounds") else ["none.jpg"]
    background_choices = [(os.path.join("backgrounds", i), i) for i in backgrounds if i.endswith("jpg")]
    background = SelectField("Background", choices=background_choices, default=background_choices[0][0])
    fonts = os.listdir("fonts") if os.path.exists("fonts") else [""]
    font_choices = [(os.path.join("fonts", i), i) for i in fonts]
    font = SelectField("Font", choices=font_choices, default=font_choices[0][0])
    reversed_arc = BooleanField("Reverse arc")
    spravka = HiddenField("Spravka", default="")
    submit = SubmitField("Send")

    def to_dict(self):
        return {
            "name": {
                "value": str(self.name.data),
                "errors": self.name.errors,
            },
            "curvature": {
                "value": float(self.curvature.data),
                "errors": self.curvature.errors,
            },
            "color_shift": {
                "value": float(self.color_shift.data),
                "errors": self.color_shift.errors,
            },
            "noise": {
                "value": float(self.noise.data),
                "errors": self.noise.errors,
            },
            "squeeze": {
                "value": float(self.squeeze.data),
                "errors": self.squeeze.errors,
            },
            "arc": {
                "value": float(self.arc.data),
                "errors": self.arc.errors,
            },
            "aspect_ratio": {
                "value": float(self.aspect_ratio.data),
                "errors": self.aspect_ratio.errors,
            },
            "shade_val": {
                "value": float(self.shade_val.data),
                "errors": self.shade_val.errors,
            },
            "mode": {
                "value": self.mode.data,
                "choices": self.mode_choices,
                "errors": self.mode.errors,
            },
            "background": {
                "value": self.background.data,
                "choices": self.background_choices,
                "errors": self.background.errors,
            },
            "font": {
                "value": self.font.data,
                "choices": self.font_choices,
                "errors": self.font.errors,
            },
            "reversed_arc": {
                "value": self.reversed_arc.data,
                "errors": self.reversed_arc.errors,
            },
            "csrf_token": {
                "value": self.csrf_token._value(),
                "errors": self.csrf_token.errors,
            },
            "_errors": self.errors,
        }


def serialize(data):
    return base64.b64encode(json.dumps(data).encode()).decode()


def check_captcha():
    spravka = request.form["spravka"]
    print(f"spravka={spravka}", file=sys.stderr)
    resp = requests.get(
        f"https://{args.captcha_api_host}/validate",
        {
            "spravka": spravka,
            "ip": request.headers.get("X-Forwarded-For-Y", "127.0.0.1")
        },
        headers={"X-Ya-Service-Ticket": tvm_client.get_service_ticket_for("antirobot")}
    )
    print(f"status_code={resp.status_code}", file=sys.stderr)
    content = resp.content.decode()
    print(f"content={content}", file=sys.stderr)
    return json.loads(content)["status"] == "ok"


@app.route('/', methods=['GET', 'POST'])
def index():
    form = MainForm()
    if request.method == 'POST':
        if not form.validate():
            flash('All fields are required.')
            return render_template('index.html', form_data=serialize(form.to_dict()))
        else:
            if not check_captcha():
                return render_template('index.html', form_data=serialize(form.to_dict()))

            name = request.form["name"]
            curvature = request.form["curvature"]
            color_shift = request.form["color_shift"]
            noise = request.form["noise"]
            background = request.form["background"]
            font = request.form["font"]
            squeeze = request.form["squeeze"]
            arc = request.form["arc"]
            aspect_ratio = request.form["aspect_ratio"]
            mode = request.form["mode"]
            shade_val = request.form["shade_val"]
            if "reversed_arc" in request.form:
                reversed_arc = True
            else:
                reversed_arc = False

            image = \
            generate(name, curvature, color_shift, noise, background, font, squeeze, arc, reversed_arc, aspect_ratio,
                     mode, shade_val)[0]
            if image is None:
                return render_template('index.html', form_data=serialize(form.to_dict()),
                                       error='Render failed, recheck parameters.')
            image = serve_pil_image(image)
            imgage_src = "data:image/jpeg;base64," + base64.b64encode(image).decode("utf-8")
            return render_template('index.html', form_data=serialize(dict(form.to_dict(), image=imgage_src)))

    elif request.method == 'GET':
        return render_template('index.html', form_data=serialize(form.to_dict()))


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=5000, type=int, help="Port")
    parser.add_argument("--captcha-api-host", default='captcha-test.yandex-team.ru', help="Captcha API Host")
    return parser.parse_args()


def main():
    global args
    args = parse_args()

    global tvm_client
    tvm_client = tp2.TvmClient(tp2.TvmApiClientSettings(
        self_client_id=2029200,
        self_secret=os.getenv("TVM_SECRET"),
        # https://yav.yandex-team.ru/secret/sec-01fafjahzbpfea0q45jrj3p4nr/explore/versions
        dsts={"antirobot": 2002152}
    ))
    app.run(debug=False, host='::', port=args.port)


if __name__ == '__main__':
    main()
