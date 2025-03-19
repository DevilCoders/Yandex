#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Flask application implementing base functionality of web service mock.
"""
import json
from functools import wraps

from flask import abort, current_app, Flask, jsonify, request, Response
from werkzeug.exceptions import default_exceptions, HTTPException


def check_auth(callback):
    """
    Mock auth check helper
    """

    @wraps(callback)
    def auth_wrapper(*args, **kwargs):
        """
        Check if OAuth token in Authorization equals
        OAUTH_TOKEN config value
        """
        token = request.headers.get('Authorization')
        if not token:
            abort(401)
        value = token.replace('OAuth ', '')
        if value != current_app.config['OAUTH_TOKEN']:
            abort(403)

        return callback(*args, **kwargs)

    return auth_wrapper


class ServerMock(Flask):
    """
    Flask application implementing base functionality of web service mock.
    """

    _mock_data = {
        'invocations': [],
        'rules': [],
        'reset_handlers': [],
    }

    def __init__(self, *args, **kwargs):
        super(ServerMock, self).__init__(*args, **kwargs)

        self.add_url_rule('/mock/ping', view_func=self._ping_handler)

        self.add_url_rule('/mock/config', view_func=self._config_handler)

        self.add_url_rule('/mock/invocations', view_func=self._get_invocations_handler)

        self.add_url_rule('/mock/rules', view_func=self._get_rules, methods=['GET'])

        self.add_url_rule('/mock/rules', view_func=self._post_rules, methods=['POST'])

        self.add_url_rule('/mock/reset', view_func=self._reset_handler, methods=['POST'])

        self.before_request(self._before_request_handler)

        self.after_request(self._after_request_handler)

        for code in default_exceptions:
            self.register_error_handler(code, self._error_handler)

    def reset_handler(self, func):
        """
        Register a cleanup function to run at /mock/reset invocation.
        """
        ServerMock._mock_data['reset_handlers'].append(func)
        return func

    def _ping_handler(self):
        """
        Check service availability.
        """
        return jsonify(status='OK')

    def _config_handler(self):
        """
        Return application config.
        """
        return Response(json.dumps(self.config, default=repr), mimetype='application/json')

    def _get_invocations_handler(self):
        """
        Return performed mock invocations.
        """
        return jsonify(ServerMock._mock_data['invocations'])

    def _post_rules(self):
        """
        Create mock rule.
        """
        json_data = request.get_json()
        if isinstance(json_data, list):
            ServerMock._mock_data['rules'].extend(json_data)
        elif isinstance(json_data, dict):
            ServerMock._mock_data['rules'].append(json_data)
        else:
            abort(422)
        return jsonify(status='OK')

    def _get_rules(self):
        """
        Return active mock rules.
        """
        return jsonify(ServerMock._mock_data['rules'])

    def _reset_handler(self):
        """
        Reset mock to the initial state.
        """
        for handler in ServerMock._mock_data['reset_handlers']:
            handler()

        for key in ('invocations', 'rules'):
            ServerMock._mock_data[key].clear()

        return jsonify(status='OK')

    def _before_request_handler(self):

        if request.path.startswith('/mock/'):
            return None

        rules = ServerMock._mock_data['rules']
        for rule in rules.copy():
            if rule.get('path') == request.path:
                status = rule.get('status_code', 200)
                response_json = rule.get('json')
                response = jsonify(response_json) if response_json else b''
                times = rule.get('times')
                if times:
                    if times == 1:
                        rules.remove(rule)
                    else:
                        rule['times'] -= 1

                return response, status

        return None

    def _after_request_handler(self, response):
        """
        Register mock invocation.
        """
        if request.path.startswith('/mock/'):
            return response

        invocation = {
            'method': request.method,
            'path': request.path,
            'args': request.args,
            'headers': dict(request.headers),
            'staus_code': response.status_code,
        }
        if request.args:
            invocation['args'] = request.args
        json_data = request.get_json(force=True, silent=True)
        if json_data:
            invocation['json'] = json_data

        ServerMock._mock_data['invocations'].append(invocation)

        return response

    def _error_handler(self, error):
        """
        Handler of HTTP errors.
        """
        error_code = getattr(error, 'code', 500)

        response = {}

        if isinstance(error, HTTPException):
            response['message'] = error.description
        else:
            response['message'] = repr(error)

        nested_error = getattr(error, 'exc', None)
        if nested_error:
            details = getattr(nested_error, 'messages')
            if details:
                response['details'] = details

        return jsonify(response), error_code
