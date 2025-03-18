import logging

from antirobot.cbb.cbb_django.cbb.library import db
from antirobot.cbb.cbb_django.cbb.models import UserRole
from django.core.management.base import BaseCommand


@db.main.use_master(commit_on_exit=True)
def grant_roles(role, logins):
    for login in logins:
        user_role = db.main.session.query(UserRole).filter_by(login=login).first()
        if user_role:
            user_role.role = role
        else:
            user_role = UserRole(login=login, role=role)
        db.main.session.add(user_role)


class Command(BaseCommand):
    help = "Grant role to user list"

    def add_arguments(self, parser):
        parser.add_argument(
            "--role",
            choices=["admin", "user", "supervisor"],
            help="Role",
        )
        parser.add_argument(
            "--login",
            help="Login",
            nargs="+",
        )

    def handle(self, *args, **options):
        logging.info(options["role"])
        logging.info(options["login"])
        grant_roles(options["role"], options["login"])

        self.stdout.write("Done\n")
