from flask import Flask, g
from flask_passport import Authenticator

app = Flask(__name__)
Authenticator(app)


@app.route("/")
def hello():
    return "Hello {}!".format(g.identity.id)


if __name__ == "__main__":
    app.run()
