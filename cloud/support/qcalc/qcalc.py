from app import app
from app.config import *
os.environ['REQUESTS_CA_BUNDLE'] = 'app/static/allCAs.pem'

if __name__ == '__main__':
    app.run(host='::',
            port=Config.PORT,
            debug=Config.DEBUG)
