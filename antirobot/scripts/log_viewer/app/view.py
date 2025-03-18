# coding: utf-8

import sys
import os
import traceback
import subprocess
import tempfile
import urllib
import logging
import types

from flask import request, url_for, render_template, Response, redirect, json, jsonify, flash

from . import app, cache
from errors import ESlowFetchRequired

import misc
import search_access
import search_events

from forms import AccesslogQueryForm, EventlogQueryForm
import data_fetcher
import yt_client
import yt_cache
import precalc_mgr

import accesslog_formatter
import eventlog_formatter

import forms

from antirobot.scripts.utils import ip_utils
from antirobot.scripts.access_log import RequestYTDict, MarketRequestYTDict


def ChooseKey(form):
    if form.ip.data:
        return precalc_mgr.IP_FIELD
    elif form.yandexuid.data:
        return precalc_mgr.YUID_FIELD
    else:
        raise Exception, 'IP or yandexuid must be specified'


def GetAccesslogFetcher(conf, form, what):
    return search_access.AccesslogFetcher(conf, ChooseKey(form), what, substring=form.substring.data, isRe=form.isre.data)


def utf8(text):
    if isinstance(text, types.UnicodeType):
        return text.encode('utf-8')

    return str(text)


def RemoveArg(args, argName):
    args = request.args.copy()
    del args[argName]

    return args


@app.route('/', methods=['GET'])
@app.route('/accesslog/<string:what>', methods=['GET'])
def ViewAccessLog(what='main'):
    form = None
    try:
        form = AccesslogQueryForm(request.args)
        form.validate()

        if (request.args.get('upd_cache')):
            GetAccesslogFetcher(app.config['MY_CONFIG'], form, what).DeleteCache(date=form.date.data, ip=form.ip.data, yuid=form.yandexuid.data)
            return redirect(url_for('ViewAccessLog', what=what, **RemoveArg(request.args, 'upd_cache')))

        doReq = False
        getDataUrl = url_for('GetAccessLog', what=what, **request.args)

        if form.doreq.data:
            doReq = (form.ip.data or form.yandexuid.data) and not form.errors

        return render_template('accesslog.html',
                form=form,
                what=what,
                doReq=doReq,
                getDataUrl=getDataUrl,
                searchBy="market's accesslog" if what == 'market' else 'accesslog'
                )
    except Exception, ex:
        print misc.FormatTraceback(traceback.format_exc())
        return render_template('accesslog.html',
                form=form,
                what=what,
                error=misc.FormatTraceback(traceback.format_exc())
                )


@app.route('/ping')
def Ping():
    return "ok"


@app.route('/ajax/accesslog/<string:what>')
def GetAccessLog(what='main'):
    conf = app.config['MY_CONFIG']
    try:
        form = AccesslogQueryForm(request.args)
        #page = misc.ToInteger(request.args.get('page'), 1)
        hint = misc.ToInteger(request.args.get('hint'), None)
        shortLog = form.shortlog.data
        updateCache = bool(request.args.get('upd_cache'))

        fetchResult = GetAccesslogFetcher(conf, form, what).GetData(
            date=form.date.data,
            ip=form.ip.data,
            yuid=form.yandexuid.data,
            slowEnable=form.slow.data,
            firstRowIndex=hint,
            updateCache=updateCache
            )

        if fetchResult.status == 'ready':
            args = request.args.copy()
            args['hint'] = fetchResult.nextRow
            return jsonify({
                'status': fetchResult.status,
                'rows': accesslog_formatter.FormatRows(conf.KEYS_FILE, fetchResult.rows, form.date.data, shortLog, reqParser=MarketRequestYTDict if what == 'market' else RequestYTDict),
                'next_url': url_for('GetAccessLog', what=what, **args),
                'at_end': fetchResult.atEnd
                })

        if fetchResult.status == 'error':
            return jsonify({
                'status': 'error',
                'descr': fetchResult.descr,
            })

        return jsonify({
            'status': 'pending',
            'rows': [],
            'next_url': url_for('CheckPending', id=fetchResult.pendingId),
            'pending': fetchResult.pendingId,
            })

    except ESlowFetchRequired:
        return jsonify({
            'status': 'error',
            'rows': [],
            'descr': 'Slow fetch is required.<br>Please check the <span class="ref">allow slow search</span> checkbox and repeat.',
            })

    except Exception as ex:
        print >>sys.stderr, 'Exception', misc.FormatTraceback(traceback.format_exc())
        return jsonify({
            'status': 'error',
            'descr': str(ex),
            'full_descr': misc.FormatTraceback(traceback.format_exc()),
            'retry_url': url_for('GetAccessLog', what=what, **request.args)
            })


def GetEventlogFetcher(conf, form):
    return search_events.EventlogFetcher(conf, ChooseKey(form), substring=form.substring.data, token=form.token.data)


@app.route('/eventlog')
def ViewEventLog():
    form = EventlogQueryForm(request.args)
    form.validate()

    if (request.args.get('upd_cache')):
        GetEventlogFetcher(app.config['MY_CONFIG'], form).DeleteCache(date=form.date.data, ip=form.ip.data, yuid=form.yandexuid.data)
        return redirect(url_for('ViewEventLog', **RemoveArg(request.args, 'upd_cache')))

    doReq = False
    page = misc.ToInteger(request.args.get('page'), 1)
    getDataUrl = url_for('GetEventLog', **request.args)

    if form.doreq.data:
        doReq = (form.ip.data or form.yandexuid.data) and not form.errors

    return render_template('eventlog.html',
            form=form,
            doReq=doReq,
            page=page,
            getDataUrl=getDataUrl
            )


@app.route('/ajax/eventlog')
def GetEventLog():
    conf = app.config['MY_CONFIG']
    try:
        form = EventlogQueryForm(request.args)
        hint = misc.ToInteger(request.args.get('hint'), None)
        updateCache = bool(request.args.get('upd_cache'))

        fetchResult = GetEventlogFetcher(conf, form).GetData(
            date=form.date.data,
            ip=form.ip.data,
            yuid=form.yandexuid.data,
            slowEnable=form.slow.data,
            firstRowIndex=hint,
            updateCache=updateCache
            )

        if fetchResult.status == 'ready':
            args = request.args.copy()
            args['hint'] = fetchResult.nextRow
            return jsonify({
                'status': fetchResult.status,
                'rows': eventlog_formatter.FormatResult(fetchResult.rows, conf.KEYS_FILE),
                'next_url': url_for('GetEventLog', **args),
                'at_end': fetchResult.atEnd
                })

        if fetchResult.status == 'error':
            return jsonify({
                'status': 'error',
                'descr': fetchResult.descr,
            })

        return jsonify({
            'status': 'pending',
            'rows': [],
            'next_url': url_for('CheckPending', id=fetchResult.pendingId),
            'pending': fetchResult.pendingId,
            })

    except ESlowFetchRequired:
        return jsonify({
            'status': 'error',
            'rows': [],
            'descr': 'Slow fetch is required.<br>Please check the <span class="ref">allow slow search</span> checkbox and repeat.',
            })

    except Exception as ex:
        print >>sys.stderr, 'Exception', misc.FormatTraceback(traceback.format_exc())
        return jsonify({
            'status': 'error',
            'descr': str(ex),
            'full_descr': misc.FormatTraceback(traceback.format_exc()),
            'retry_url': url_for('GetEventLog', **request.args)
            })


@app.route('/ajax/check_pending')
def CheckPending():
    conf = app.config['MY_CONFIG']
    id = request.args.get('id')
    if not id:
        return jsonify({'status': 'error', 'descr': 'Invalid pending id'})

    result = data_fetcher.DataFetcher.GetPendingInfo(id)

    return jsonify(result.Repr())


@app.route('/clearcache', methods=['GET', 'POST'])
def ClearAllCache():
    conf = app.config['MY_CONFIG']
    if request.method == 'POST':
        cache.ClearAll()
        return render_template('clear_cache.html', done=True)

    return render_template('clear_cache.html')


@app.route('/setbancookie', methods=['GET', 'POST'])
def SetBanCookie():
    host = request.headers.get('host').split(':')[0]
    domain = '.' + '.'.join(host.split('.')[-2:])

    if request.method == 'POST':

        #return redirect('/setbancookie?done=1')
        resp = redirect('/setbancookie?done=1')
        resp.set_cookie('YX_SHOW_CAPTCHA', '1', domain=domain, expires='Wed, 01-Jan-2020 01:00:00 GMT')
        #resp = make_response(redirect('/setbancookie?done=1'))
        return resp

    return render_template('set_ban_cookie.html', done=request.args.get('done'), domain=domain)


@app.route('/ip2backend', methods=['GET', 'POST'])
def ip2backend():
    form = forms.Ip2BackendForm()
    form_valid = form.validate()
    return_code = None
    result = None

    if form_valid:
        ip2backend_bin = os.path.join(app.config['MY_CONFIG'].APP_BIN_PATH, 'ip2backend')
        args = [ip2backend_bin, form.ip.data]
        if form.options.data:
            args.extend(form.options.data.split())

        p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (out, err) = p.communicate()
        return_code = p.returncode
        result = out if return_code == 0 else err

    return render_template('ip2backend.html',
        form_valid = form_valid,
        return_code = return_code,
        result = result,
        form=form
        )


DEF_RULE = u"doc=/\/search\.*/"
DEF_REQUEST = u"""GET /search/?text=котики HTTP/1.1
Host: yandex.ru
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.71 Safari/537.17 YE
Accept-Encoding: gzip,deflate,sdch
Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4
Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3
Cookie: yandexuid=9734851951328192577
X-Forwarded-For-Y: 1.2.3.4
X-Source-Port-Y: 32736
X-Start-Time: 1391611472705260
X-Req-Id: 1391611472705260-17432647174108823325
"""

@app.route('/check_addos_rule')
def check_addos_rule():
    rule = request.form.get('rules') or DEF_RULE
    req_text = request.form.get('request') or DEF_REQUEST

    return render_template('antiddos_tool.html', rule=rule, request=req_text)


@app.route('/ajax/check_addos_rule', methods=['POST'])
def ajax_check_addos_rule():
    rules = request.form.get('rules')
    print >>sys.stderr, "rules:", rules
    return_code = None
    result = None

    try:
        if not rules:
            raise Exception("Inavlid params")

        antiddos_bin = os.path.join(app.config['MY_CONFIG'].APP_BIN_PATH, 'antiddos')
        args = [antiddos_bin, '--json', 'parse', '-']

        p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (out, err) = p.communicate(rules)
        return_code = p.returncode
        if return_code != 0:
            raise Exception('antiddos_tool returned non-zero value. Something went wrong.')

        return jsonify(json.loads(out))

    except Exception, ex:
        logging.exception(ex)
        return jsonify({'success': False, 'message': str(ex)})


@app.route('/ajax/check_addos_request', methods=['POST'])
def ajax_check_addos_request():
    rules = utf8(request.form.get('rules'))
    req_text = utf8(request.form.get('request'))

    return_code = None
    result = None

    try:
        if not rules  or not req_text:
            raise Exception("Inavlid params")

        req_file = tempfile.NamedTemporaryFile()
        req_file.write(urllib.quote(req_text) + '\n')
        req_file.flush()

        antiddos_bin = os.path.join(app.config['MY_CONFIG'].APP_BIN_PATH, 'antiddos')
        args = [
            antiddos_bin,
            '--json',
            '--req-log-txt',
            '--req-log', req_file.name,
            '--stat',
            'check',
            '-'
        ]

        p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (out, err) = p.communicate(rules)
        return_code = p.returncode
        if return_code != 0:
            raise Exception('antiddos_tool returned non-zero value. Something went wrong.')

        result = {
            'success': True,
            'message': 'Success',
            'matched': 0,
        }

        addos_res = json.loads(out)
        if 'success' in addos_res:
            result['success'] = addos_res['success']
            result['message'] = addos_res['message']
            return jsonify(result)

        if 'matched' not in addos_res:
            result['success'] = False
            result['message'] = 'Unknown response from antiddos tool'
            return jsonify(result)

        result['matched'] = addos_res['matched']
        return jsonify(result)

    except Exception, ex:
        logging.exception(ex)
        return jsonify({'success': False, 'message': str(ex)})
