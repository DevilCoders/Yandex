import datetime
import time

from dateutil.relativedelta import relativedelta

import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Boolean, Column, DateTime, Integer, String, Text
from sqlalchemy.dialects.postgresql import ENUM as Enum

DefaultEnum = Enum("0", "1", "2", "3", "4", name="default_enum")


class Group(db.main.Base):

    __tablename__ = "cbb_groups"

    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(String(255), nullable=False, default="")
    group_descr = Column(Text, nullable=False, default="")
    updated = Column(DateTime, nullable=True)
    created = Column(DateTime, default=datetime.datetime.now, nullable=False)
    default_type = Column(DefaultEnum, default="1", nullable=False)
    is_active = Column(Boolean, default=True, nullable=False)
    is_internal = Column(Boolean, default=False, nullable=False)
    period_to_expire = Column(Integer, default=0)

    def __repr__(self):
        return "<Group %r, id: %r>" % (self.name, self.id)

    def kind(self):
        if self.default_type == "3":
            return "txt"
        elif self.default_type == "4":
            return "re"
        else:
            return "ip"

    def get_period_to_expire(self):
        """
        Returns formatted string from timestamp
        """
        ts = self.period_to_expire

        if not ts:
            return ""
        time = []
        delta = relativedelta(seconds=ts)
        if delta.days:
            time.append("%dd" % delta.days)
        if delta.hours:
            time.append("%dh" % delta.hours)
        if delta.minutes:
            time.append("%dm" % delta.minutes)
        return " ".join(time)

    def has_undeleted_ranges(self):
        """
        Check if group has any undeleted ranges
        """
        from . import BLOCK_VERSIONS
        for RangeClass in BLOCK_VERSIONS.values():
            if RangeClass.query.filter_by(group_id=self.id).first():
                return True
        return False

    def has_ip_from_links(self):
        from . import BLOCK_VERSIONS

        ip_from_str = f"ip_from={self.id}"

        for gr in Group.query.filter_by(is_active=True).order_by(Group.id).all():
            if gr.kind() == "txt":
                for rng in BLOCK_VERSIONS["txt"].q_find(groups=[gr.id]):
                    if ip_from_str in rng.rng_txt.split(";"):
                        return True

        return False

    def increase_expire(self, expire):
        if not expire and self.period_to_expire:
            expire = datetime.datetime.now() + datetime.timedelta(seconds=self.period_to_expire)
        return expire

    def update_updated(self):
        """
        Replace updated if it"ll change seconds
        """
        time_now = datetime.datetime.now().replace(microsecond=0)
        my_time = None
        if self.updated:
            my_time = self.updated.replace(microsecond=0)
        if my_time != time_now:
            self.updated = time_now

    @classmethod
    def get_internal_ids(cls, group_types=("0", "1", "2")):
        groups = db.main.session.query(Group.id).filter_by(is_internal=True, is_active=True).filter(Group.default_type.in_(group_types)).all()
        return [group[0] for group in groups]

    @classmethod
    def get_groups_updated(cls, group_ids):
        """
        Returns {group.id: group.updated} pairs for group_ids
        """
        if not group_ids:
            return

        groups_updated = dict(db.main.session.query(Group.id, Group.updated).filter(Group.id.in_(group_ids)).all())
        return groups_updated

    @classmethod
    def latest_group_update_time(cls, group_ids):
        """
        Returns float timestamp of the latest groups updated time
        """
        if not group_ids:
            return

        groups_updated = Group.get_groups_updated(group_ids)
        upd_times = [x for x in groups_updated.values() if x]

        if not upd_times:
            return

        last_time = max(upd_times)
        # make last_time to float timestamp
        return last_time and (time.mktime(last_time.timetuple()) + last_time.microsecond / 1E6)

    @classmethod
    def get_service_active_groups(cls, service=None, default_types=None):
        """
        Returns active groups' ids that belongs certain service
        """
        if service is None:
            return

        groups = db.main.session.query(Group.id, Group.group_descr).filter_by(is_active=True)
        if default_types:
            groups = groups.filter(cls.default_type.in_(default_types))

        if service:
            groups = [gr for gr in groups.all() if service in gr[1].split(' ')]

        return [group[0] for group in groups]
