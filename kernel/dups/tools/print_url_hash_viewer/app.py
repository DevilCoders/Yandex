import flask
from flask import request
import jinja2
import util
import mimetypes
import collections

from urlnorm import GetHashKeyForUrl


app = flask.Flask(__name__)
app.jinja_loader = jinja2.DictLoader(dict(util.load_resources("/jinja/")))
app.static_resources = dict(util.load_resources("/res/"))


colors_num = 27


@app.route('/res/<path:filepath>')
def static_res(filepath):
    """ 'static' files handler using built-in resources instead of files """
    if filepath not in app.static_resources:
        flask.abort(404)
    r = flask.make_response(app.static_resources[filepath])
    guess_result = mimetypes.guess_type(filepath)
    r.mimetype = guess_result[0]
    r.cache_control.max_age = 300
    return r


@app.route('/docid', methods=['GET'])
def s():
    urls_text = request.args.get('urls', '')
    urls = map(str.strip, urls_text.encode('utf-8').split())
    hashes = map(GetHashKeyForUrl, urls)
    hash_cnt = collections.Counter(hashes)

    r = []
    non_uniq_group_hashes = {}
    for u, hash in zip(urls, hashes):
        if hash_cnt[hash] == 1:
            uniq_hash_id = None
            groupId = 'groupUniq'
            hash_sym = 'uniq'
        else:
            uniq_hash_id = non_uniq_group_hashes.setdefault(hash, len(non_uniq_group_hashes))
            groupId = 'group{}'.format((17 * uniq_hash_id) % colors_num)
            hash_sym = uniq_hash_id

        r.append({
            'url': u.decode('utf-8'),
            'hash': hash,
            'hash_sym': hash_sym,
            'class': groupId,
        })

    render = flask.render_template('main.html', url_ids=r, prev_urls_text=urls_text)
    resp = flask.make_response(render)
    resp.cache_control.max_age = 30
    return resp
