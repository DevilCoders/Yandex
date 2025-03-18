from datetime import datetime

from antiadblock.configs_api.lib.db import AuditEntry, db


def log_action(action, user_id, service_id=None, label_id=None, **params):
    entry = AuditEntry(date=datetime.utcnow(),
                       action=action,
                       params=params,
                       user_id=user_id,
                       service_id=service_id,
                       label_id=label_id)
    db.session.add(entry)
