import os
import sys

FILE_DIRECTORY = os.path.dirname(os.path.realpath(__file__))
DEPLOY_SCRIPT = os.path.join(FILE_DIRECTORY, '..', '..', '..', 'devtools', 'maven-deploy', 'deploy.py')

cmd = [
    sys.executable,
    DEPLOY_SCRIPT,
    os.path.join('library', 'java', 'monlib')
]

os.execv(sys.executable, cmd)
