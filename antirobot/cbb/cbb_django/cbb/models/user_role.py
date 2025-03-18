from antirobot.cbb.cbb_django.cbb.library.db import main
from sqlalchemy import Column, Integer, String, Index
from sqlalchemy.dialects.postgresql import ENUM as Enum
from django.conf import settings


RoleEnum = Enum(*settings.ROLES.keys(), name="role_enum")


class UserRole(main.Base):
    __tablename__ = "cbb_user_role"
    __table_args__ = (Index("ix_login_role", "login", "role"),)

    login = Column(String(128), nullable=False, primary_key=True)
    role = Column(RoleEnum, nullable=False)


class GroupResponsible(main.Base):
    __tablename__ = "cbb_group_responsible"

    group_id = Column(Integer, primary_key=True)
    user_login = Column(String(32), primary_key=True)

    def __repr__(self):
        return "<GroupResponsible %r, %r>" % (self.group_id, self.user_login)

    @classmethod
    def get_responsibles(cls, group_id):
        if not group_id:
            return []

        rows = main.session.query(GroupResponsible.user_login).filter_by(group_id=group_id).all()
        return [row.user_login for row in rows]

    # @classmethod
    # def get_subject_groups(cls, user_login):
    #     if not user_login:
    #         return None

    #     user_login = user_login.strip().lower()
    #     rows = main.session.query(Group.id).filter_by(user_login=group_id).all()
    #     return set((row.group_id for row in rows))
