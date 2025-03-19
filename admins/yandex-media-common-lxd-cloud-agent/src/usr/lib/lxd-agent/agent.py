import connexion
from connexion.resolver import RestyResolver

app = connexion.FlaskApp(__name__, specification_dir='v1')
app.add_api('spec.yaml',resolver=RestyResolver('v1'))
