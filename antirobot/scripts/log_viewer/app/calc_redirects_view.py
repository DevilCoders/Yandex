from flask import render_template, Response
from . import app

import yt_client
import calc_redirects

def GetYT():
    conf = app.config['MY_CONFIG']
    return yt_client.GetYtClient(conf.YT_PROXY, conf.YT_TOKEN)


@app.route('/redirect_counts', methods=['GET'])
def ShowRedirectCounts():
    MAX_WEEKS_TO_SHOW = 56 # last year
    conf = app.config['MY_CONFIG']
    result = {
        'status': 'ready',
        'data': calc_redirects.ReadTargetTable(GetYT(), conf.CLUSTER_ROOT, maxItems=MAX_WEEKS_TO_SHOW)
    }

    return render_template('redirect_counts.html', result=result)


@app.route('/last_redirect_counts', methods=['GET'])
def ShowLastRedirectCounts():
    conf = app.config['MY_CONFIG']
    result = [x for x in calc_redirects.ReadTargetTable(GetYT(), conf.CLUSTER_ROOT, maxItems=1)]
    if result:
        result = result[0]
    else:
        result = ('', '', '')

    response = \
'''<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<redirect_counts>
    <period>%s</period>
    <value>%s</value>
</redirect_counts>
''' % (result[0], result[2])

    return Response(
        response,
        mimetype='text/xml'
        )
