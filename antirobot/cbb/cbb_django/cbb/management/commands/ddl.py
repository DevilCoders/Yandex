# coding: utf-8

from django.core.management.base import BaseCommand, CommandError
from sqlalchemy import create_engine

from antirobot.cbb.cbb_django.cbb.library import db


class Command(BaseCommand):
    help = "Print ddl for creating tables"

    def add_arguments(self, parser):
        parser.add_argument('--base')

    def handle(self, *args, **options):
        if not options["base"]:
            raise CommandError("Specify base type")

        def dump(sql, *multiparams, **params):
            print(sql.compile(dialect=engine.dialect), ";")

        engine = create_engine("postgresql://", strategy="mock", executor=dump)

        if options["base"] == "main":
            print("CREATE SCHEMA cbb;")
            db.main.meta.create_all(engine, checkfirst=False)
            return

        if options["base"] == "history":
            print("CREATE SCHEMA cbb_history;")
            db.history.meta.create_all(engine, checkfirst=False)
            return
