# coding=utf-8
from datetime import datetime
from flask import Blueprint, jsonify, request

from antiadblock.configs_api.lib.auth.auth import ensure_tvm_ticket_is_ok
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.db import db, BotChatConfig, BotEvent

bot_api = Blueprint('bot_api', __name__)

ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID = 'ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID'


@bot_api.route('/get_bot_configs', methods=['GET'])
def get_bot_config():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=[ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID])
    configs = BotChatConfig.query.all()
    return jsonify({'result': [config.as_dict() for config in configs]})


@bot_api.route('/register_bot_chat', methods=['POST'])
def register_bot_chat():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=[ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID])
    data = request.get_json(force=True)
    chat_config = BotChatConfig(chat_id=data['chat_id'], service_id=data['service_id'],
                                config_notification=data.get('config_notification', False),
                                release_notification=data.get('release_notification', False),
                                rules_notification=data.get('rules_notification', False),
                                )
    db.session.add(chat_config)
    db.session.commit()
    return "{}", 201


@bot_api.route('/post_bot_event', methods=['POST'])
def post_bot_event():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=[ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID])
    data = request.get_json(force=True)
    event = BotEvent(
        event_date=datetime.utcnow(),
        event_type=data['event_type'],
        data=data['data']
    )
    db.session.add(event)
    db.session.commit()
    return "{}", 201


@bot_api.route('/get_bot_events', methods=['GET'])
def get_events():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=[ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID])
    return jsonify({'result': [event.as_dict() for event in BotEvent.query.all()]})


@bot_api.route('/delete_bot_event/<int:id>', methods=['GET'])
def delete_bot_event(id):
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=[ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID])
    event = BotEvent.query.filter(id=id).first_or_404()
    db.session.delete(event)
    db.session.commit()
    return "{}", 201
