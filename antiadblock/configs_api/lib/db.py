# coding=utf8
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy import Enum, inspect
from sqlalchemy.dialects.postgresql import ARRAY, JSONB
from sqlalchemy.ext.declarative.clsregistry import _ModuleMarker
from sqlalchemy.orm import RelationshipProperty

from antiadblock.configs_api.lib.models import ConfigStatus, ServiceStatus, ServiceSupportPriority, AuditAction,\
    CheckState, SbSCheckStatus

db = SQLAlchemy()


class Config(db.Model):
    """
    Database representation of config. It is used to provide frequently changed settings to cry proxy. One database unit
    of settings expected to be immutable. When one need to make changes to this settings new database record is created
    """
    __tablename__ = 'configs'
    id = db.Column(db.Integer, primary_key=True)
    comment = db.Column(db.String(60), nullable=False)
    data = db.Column(JSONB, nullable=False)
    # в этом поле для каждого ключа будем сохранять параметры зануления https://st.yandex-team.ru/ANTIADB-2063
    # data = {'key': {'UNSET': True}, ...}
    data_settings = db.Column(JSONB, nullable=False)
    service_id = db.Column(db.String(127), db.ForeignKey('services.id'), nullable=True)
    created = db.Column(db.DateTime, nullable=False)
    creator_id = db.Column(db.BigInteger, nullable=True)
    # предыдущий конфиг в истории
    parent_id = db.Column(db.Integer, db.ForeignKey('configs.id'), nullable=True)
    # родительский конфиг
    parent_label_id = db.Column(db.String(127), nullable=True)
    # для какого эксперимента конфиг
    exp_id = db.Column(db.String(127), nullable=True)
    # имя для отображения в иерархии
    label_id = db.Column(db.String(127), nullable=False)
    archived = db.Column(db.BOOLEAN, default=False)
    statuses = db.relationship('ConfigMark', backref='config', lazy=True)

    def as_dict(self, with_statuses=False):
        result = {
            'id': self.id,
            'comment': self.comment,
            'data': self.data,
            'data_settings': self.data_settings,
            'service_id': self.service_id,
            'created': self.created,
            'archived': self.archived,
            'creator_id': self.creator_id,
            'parent_id': self.parent_id,
            'parent_label_id': self.parent_label_id,
            'exp_id': self.exp_id,
            'label_id': self.label_id,
        }
        # if statuses were loaded
        if "statuses" in self.__dict__ or with_statuses:
            statuses = map(lambda cm: cm.as_dict(), self.statuses)
            result.update(dict(statuses=statuses))
        return result

    def get_statuses(self):
        return map(lambda cm: cm.status, self.statuses)


db.Index("idx_config_service_id_created", Config.service_id, Config.created)


class ConfigMark(db.Model):

    __tablename__ = 'config_statuses'
    config_id = db.Column(db.Integer, db.ForeignKey('configs.id'), primary_key=True)
    status = db.Column(Enum(*ConfigStatus.all(), name="config_status"), nullable=False, primary_key=True)
    comment = db.Column(db.String(2048), nullable=True)

    def as_dict(self):
        return {
            'status': self.status,
            'comment': self.comment,
        }


class Service(db.Model):
    """
    Database representation of antiadblock service. It's means to be used instead of "partner" and represent service aka
    group of domains with similar functionality
    """
    __tablename__ = 'services'
    id = db.Column(db.String(127), primary_key=True, autoincrement=False)
    name = db.Column(db.String(60), nullable=False)
    configs = db.relationship('Config', backref='service', lazy=True)
    audit_entries = db.relationship('AuditEntry', backref='service', lazy=True)
    status = db.Column(Enum(*ServiceStatus.all(), name="service_status"), nullable=False, default=ServiceStatus.INACTIVE)
    domain = db.Column(db.String(127), nullable=False, unique=True)
    owner_id = db.Column(db.BigInteger, nullable=True)
    monitorings_enabled = db.Column(db.BOOLEAN, default=True)
    mobile_monitorings_enabled = db.Column(db.BOOLEAN, default=True)
    support_priority = db.Column(Enum(*ServiceSupportPriority.all(), name="support_priority"), nullable=False, default=ServiceSupportPriority.OTHER)
    checks = db.relationship('ServiceChecks', backref='service', lazy=True)

    def as_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            # Do not trigger lazy configs loading
            # 'configs': self.configs,
            'status': self.status,
            'owner_id': self.owner_id,
            'domain': self.domain,
            'monitorings_enabled': self.monitorings_enabled,
            'mobile_monitorings_enabled': self.mobile_monitorings_enabled,
            'support_priority': self.support_priority,
        }


db.Index("idx_service_owner_id", Service.owner_id)


class AutoredirectService(db.Model):
    __tablename__ = 'autoredirect_services'
    id = db.Column(db.BIGINT, primary_key=True)
    webmaster_service_id = db.Column(db.String(512), nullable=False, unique=True)
    domain = db.Column(db.String(512), nullable=False, unique=True)
    urls = db.Column(JSONB, nullable=False, default=list())

    def as_dict(self):
        return {
            'id': self.id,
            'webmaster_service_id': self.webmaster_service_id,
            'domain': self.domain,
            'urls': self.urls,
        }


class AuditEntry(db.Model):
    __tablename__ = 'audit_log'
    id = db.Column(db.BIGINT, primary_key=True)
    user_id = db.Column(db.BigInteger)  # NULL - is system user
    date = db.Column(db.DateTime, nullable=False)
    service_id = db.Column(db.String(127), db.ForeignKey('services.id'), nullable=True)
    label_id = db.Column(db.String(127), nullable=True)
    action = db.Column(Enum(*AuditAction.all(), name="audit_action"), nullable=False)
    params = db.Column(JSONB, nullable=False, default=dict())

    def as_dict(self):
        return {
            'id': self.id,
            'user_id': self.user_id,
            'date': self.date,
            'service_id': self.service_id,
            'label_id': self.label_id,
            'action': self.action,
            'params': self.params,
        }


db.Index("idx_audit_entry_service_id_date", AuditEntry.service_id, AuditEntry.date.desc())


class UserLogins(db.Model):
    __tablename__ = 'user_logins'
    uid = db.Column(db.BIGINT, primary_key=True)
    passportlogin = db.Column(db.String(127), nullable=False)
    internallogin = db.Column(db.String(127), nullable=False)
    permissions = db.relationship('Permission', backref='user_logins')

    def as_dict(self, with_permissions=False):
        result = {
            'uid': self.uid,
            'passportlogin': self.passportlogin,
            'internallogin': self.internallogin,
        }
        # if permissions were loaded
        if "permissions" in self.__dict__ or with_permissions:
            result.update(dict(permissions=self.get_permissions()))
        return result

    def get_permissions(self):
        return map(lambda p: dict(role=p.role, node=p.node), self.permissions)


db.Index("idx_user_logins_uid", UserLogins.uid)


class Permission(db.Model):
    __tablename__ = 'permissions'
    uid = db.Column(db.BIGINT, db.ForeignKey('user_logins.uid'), primary_key=True)
    role = db.Column(db.String(63), nullable=False)
    node = db.Column(db.String(127), primary_key=True)


class ServiceComments(db.Model):
    __tablename__ = 'service_comments'
    service_id = db.Column(db.String(127), db.ForeignKey('services.id'), primary_key=True)
    comment = db.Column(db.Text)

    def as_dict(self):
        return {
            'service_id': self.service_id,
            'comment': self.comment,
        }


class ServiceChecks(db.Model):
    __tablename__ = 'service_checks'
    service_id = db.Column(db.String(127), db.ForeignKey('services.id'), primary_key=True)
    group_id = db.Column(db.String(127), nullable=False)
    check_id = db.Column(db.String(127), primary_key=True)
    state = db.Column(Enum(*CheckState.all(), name="check_state"), nullable=False)
    value = db.Column(db.String(255), nullable=False)
    external_url = db.Column(db.String(4096), nullable=False)
    last_update = db.Column(db.DateTime, nullable=False)
    valid_till = db.Column(db.DateTime, nullable=False)
    transition_time = db.Column(db.DateTime, nullable=False)

    def as_dict(self):
        return {
            'service_id': self.service_id,
            'group_id': self.group_id,
            'check_id': self.check_id,
            'state': self.state,
            'value': self.value,
            'external_url': self.external_url,
            'last_update': self.last_update,
            'valid_till': self.valid_till,
            'transition_time': self.transition_time,
        }


db.Index("idx_service_checks_service_id_check_id", ServiceChecks.service_id, ServiceChecks.check_id)


class ChecksInProgress(db.Model):
    __tablename__ = 'checks_in_progress'
    service_id = db.Column(db.String(127), primary_key=True)
    check_id = db.Column(db.String(127), primary_key=True)
    login = db.Column(db.String(63), nullable=False)
    time_from = db.Column(db.DateTime, nullable=False)
    time_to = db.Column(db.DateTime, nullable=False)
    checks_in_progress_fkey = db.ForeignKeyConstraint(['service_id', 'check_id'],
                                                      ['ServiceChecks.service_id', 'ServiceChecks.check_id'])

    def as_dict(self):
        return {
            'service_id': self.service_id,
            'check_id': self.check_id,
            'login': self.login,
            'time_in': self.time_in,
            'time_to': self.time_to,
        }


class SBSRuns(db.Model):
    __tablename__ = 'sbs_runs'
    id = db.Column(db.BIGINT, primary_key=True)
    status = db.Column(Enum(*SbSCheckStatus.all(), name="sbs_check_state"), nullable=False)
    owner = db.Column(db.BIGINT, db.ForeignKey('user_logins.uid'), nullable=False)
    date = db.Column(db.DateTime, nullable=False)
    config_id = db.Column(db.Integer, db.ForeignKey('configs.id'), nullable=False)
    sandbox_id = db.Column(db.BIGINT, nullable=False)
    profile_id = db.Column(db.BIGINT, db.ForeignKey('sbs_profiles.id'), nullable=False)
    is_test_run = db.Column(db.BOOLEAN, default=False)

    def as_dict(self):
        return {
            'id': self.id,
            'status': self.status,
            'owner': self.owner,
            'date': self.date,
            'config_id': self.config_id,
            'sandbox_id': self.sandbox_id,
            'profile_id': self.profile_id,
            'is_test_run': self.is_test_run,
        }


class SBSResults(db.Model):
    __tablename__ = 'sbs_results'
    id = db.Column(db.BIGINT, db.ForeignKey('sbs_runs.id'), primary_key=True)
    start_time = db.Column(db.DateTime, nullable=False)
    end_time = db.Column(db.DateTime, nullable=False)
    cases = db.Column(JSONB, nullable=False, default=list())
    filters_lists = db.Column(JSONB, nullable=False, default=list())

    def as_dict(self):
        return {
            'id': self.id,
            'start_time': self.start_time,
            'end_time': self.end_time,
            'cases': self.cases,
            'filters_lists': self.filters_lists
        }


class SBSProfiles(db.Model):
    __tablename__ = 'sbs_profiles'
    id = db.Column(db.BIGINT, primary_key=True)
    service_id = db.Column(db.String(127), db.ForeignKey('services.id'), nullable=False)
    date = db.Column(db.DateTime, nullable=False)
    tag = db.Column(db.String(63), default="default", nullable=False)
    is_archived = db.Column(db.BOOLEAN, default=False)
    url_settings = db.Column(JSONB, nullable=False, default=list())
    general_settings = db.Column(JSONB, nullable=False, default=dict())

    def as_dict(self):
        return {
            'id': self.id,
            'service_id': self.service_id,
            'date': self.date,
            'tag': self.tag,
            'is_archived': self.is_archived,
            'data': {
                'general_settings': self.general_settings,
                'url_settings': self.url_settings,
            },
        }


class DBLocks(db.Model):
    """
    table for using 'select for update' on its rows for acquiring locks
    """
    __tablename__ = 'db_locks'
    id = db.Column(db.String(255), primary_key=True)

    def as_dict(self):
        return {
            'id': self.id,
        }


class BotChatConfig(db.Model):
    """
    table for storing bot chat config
    """
    __tablename__ = 'bot_chat_config'
    chat_id = db.Column(db.String(), primary_key=True, nullable=False, unique=True)
    service_id = db.Column(db.String(127), db.ForeignKey('services.id'), nullable=False, unique=True)
    config_notification = db.Column(db.BOOLEAN, default=False)
    release_notification = db.Column(db.BOOLEAN, default=False)
    rules_notification = db.Column(db.BOOLEAN, default=False)

    def as_dict(self):
        return {
            'chat_id': self.chat_id,
            'service_id': self.service_id,
            'config_notification': self.config_notification,
            'release_notification': self.release_notification,
            'rules_notification': self.rules_notification,
        }


class BotEvent(db.Model):
    """
    table for bot event bus
    """
    __tablename__ = 'bot_event'
    id = db.Column(db.BIGINT, primary_key=True, nullable=False, unique=True)
    event_date = db.Column(db.DateTime, nullable=False)
    event_type = db.Column(db.String(), nullable=False)
    data = db.Column(JSONB, nullable=False)

    def as_dict(self):
        return {
            'id': self.id,
            'event_date': self.event_date,
            'event_type': self.event_type,
            'data': self.data,
        }


def init_database(app, db):
    app.logger.info("Insuring schema created")

    def is_sane_database(Base, session):
        """Check whether the current database matches the models declared in model base.

        Currently we check that all tables exist with all columns. What is not checked

        * Column types are not verified

        * Relationships are not verified at all (TODO)

        :param Base: Declarative Base for SQLAlchemy models to check

        :param session: SQLAlchemy session bound to an engine

        :return: True if all declared models have corresponding tables and columns.
        """

        engine = session.get_bind()
        iengine = inspect(engine)

        errors = False

        tables = iengine.get_table_names()

        # Go through all SQLAlchemy models
        for name, klass in Base._decl_class_registry.items():

            if isinstance(klass, _ModuleMarker):
                # Not a model
                continue

            table = klass.__tablename__
            if table in tables:
                # Check all columns are found
                # Looks like [{'default': "nextval('sanity_check_test_id_seq'::regclass)", 'autoincrement': True, 'nullable': False, 'type': INTEGER(), 'name': 'id'}]

                columns = [c["name"] for c in iengine.get_columns(table)]
                mapper = inspect(klass)

                for column_prop in mapper.attrs:
                    if isinstance(column_prop, RelationshipProperty):
                        # TODO: Add sanity checks for relations
                        pass
                    else:
                        for column in column_prop.columns:
                            # Assume normal flat column
                            if column.key not in columns:
                                app.logger.error("Model {} declares column {} which does not exist in database {}"
                                                 .format(klass, column.key, engine))
                                errors = True
            else:
                app.logger.error("Model {} declares table {} which does not exist in database {}".format(klass, table, engine))
                errors = True

        return not errors
    if not is_sane_database(db.Model, db.session):
        raise Exception("Database schema doesn't corespond to ORM Model")
    app.logger.info("database schema coresponds to ORM Model. Status: OK")
