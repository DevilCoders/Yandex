# -*- coding: utf-8 -*-
import logging
import os

import click

from library.python import oauth
from library.python import vault_client as yav
from library.python.monitoring.solo.controller import Controller


CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])
# by default solo uses "SOLO" OAuth app registered by robot-yabs-rt@yandex-team.ru
DEFAULT_OAUTH_APP_CLIENT_ID = "2f9d7a6b369d4cb1b331e3985b99ed60"
DEFAULT_OAUTH_APP_CLIENT_SECRET = "27a9f33936de4b1d8fbb3ffaa5d3e3ff"

logger = logging.getLogger(__name__)


def find_tokens(tokens, tokens_secret_id):
    """
    You can get "SOLO" OAuth app token via
    https://oauth.yandex-team.ru/authorize?response_type=token&client_id=2f9d7a6b369d4cb1b331e3985b99ed60
    """
    for token_key in tokens.keys():
        if os.getenv("SOLO_TOKEN", False):
            tokens[token_key] = os.getenv("SOLO_TOKEN")
        elif os.getenv(token_key, False):
            tokens[token_key] = os.getenv(token_key)
        elif tokens_secret_id:
            secret_data = yav.instances.Production().get_version(tokens_secret_id)["value"]
            for key in ["SOLO_TOKEN", token_key]:
                if key in secret_data:
                    tokens[token_key] = secret_data[key]
                    break
            else:
                raise Exception("\"tokens_secret_id\" is provided, but there is no {0} field in the requested secret".format(token_key))
        else:
            try:
                tokens[token_key] = oauth.get_token(DEFAULT_OAUTH_APP_CLIENT_ID, DEFAULT_OAUTH_APP_CLIENT_SECRET)
            except KeyError:
                logger.exception("Can\'t get token {0} via library.python.oauth".format(token_key))

        if tokens[token_key] is None:
            raise Exception("Can't find token: {0}".format(token_key))
    return tokens


LOG_INFO_FORMAT = "%(message)s"
LOG_DEBUG_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"


def configure_logging(verbose):
    root_logger = logging.getLogger("library.python.monitoring.solo")  # root library logger
    root_logger.setLevel(level=logging.DEBUG if verbose else logging.INFO)
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter(LOG_DEBUG_FORMAT if verbose else LOG_INFO_FORMAT)
    console_handler.setFormatter(formatter)
    root_logger.handlers = []
    root_logger.addHandler(console_handler)


def prettify_objects(
    objects_registry
):
    pass


class SolomonEndpoint(object):
    PRODUCTION = "http://solomon.yandex.net/"
    PRESTABLE = "http://solomon-prestable.yandex.net/"
    TESTING = "http://solomon-test.yandex.net/"


class JugglerEndpoint(object):
    PRODUCTION = "http://juggler-api.search.yandex.net"


def basic_run_v2(
    objects_registry,
    juggler_mark=None,
    tokens_secret_id=None,
    apply_changes=False,
    delete_untracked=False,
    clear_untracked_in_state=False,
    verbose=False,
    state_file_path=None,
    state_locke_path=None,
    state_yt_cluster="locke",
    run_prettify_objects=True,
    solomon_endpoint=SolomonEndpoint.PRODUCTION,
    juggler_endpoint=JugglerEndpoint.PRODUCTION,
):
    tokens = {
        "SOLOMON_TOKEN": None,
        "JUGGLER_TOKEN": None
    }
    if state_locke_path:
        tokens["YT_TOKEN"] = None
    find_tokens(tokens, tokens_secret_id)
    configure_logging(verbose)
    logger.info("solo - solomon and juggler configurator")

    controller = Controller(
        save=apply_changes,
        delete_untracked=delete_untracked,
        clear_untracked_in_state=clear_untracked_in_state,
        state_file_path=state_file_path,
        state_locke_path=state_locke_path,
        state_locke_token=tokens.get("YT_TOKEN", None),
        state_yt_cluster=state_yt_cluster,
        solomon_token=tokens["SOLOMON_TOKEN"],
        juggler_token=tokens["JUGGLER_TOKEN"],
        juggler_mark=juggler_mark,
        solomon_endpoint=solomon_endpoint,
        juggler_endpoint=juggler_endpoint,
    )

    if run_prettify_objects:
        prettify_objects(objects_registry)
    controller.process(objects_registry)


def build_basic_cli_v2(
    objects_registry,
    juggler_mark=None,
    tokens_secret_id=None,
    state_file_path=None,
    state_locke_path=None,
    state_yt_cluster="locke",
    run_prettify_objects=True,
    solomon_endpoint=SolomonEndpoint.PRODUCTION,
    juggler_endpoint=JugglerEndpoint.PRODUCTION,
):
    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option("--apply-changes", is_flag=True, help="apply changes on the run")
    @click.option("--delete-untracked", is_flag=True, help="delete untracked objects")
    @click.option("--clear-untracked-in-state", is_flag=True, help="clear untracked in state, do not delete")
    @click.option("--verbose", is_flag=True, help="turn on verbose logging")
    def cli(apply_changes, delete_untracked, clear_untracked_in_state, verbose):
        basic_run_v2(
            objects_registry=objects_registry,
            juggler_mark=juggler_mark,
            tokens_secret_id=tokens_secret_id,
            apply_changes=apply_changes,
            delete_untracked=delete_untracked,
            clear_untracked_in_state=clear_untracked_in_state,
            verbose=verbose,
            state_file_path=state_file_path,
            state_locke_path=state_locke_path,
            state_yt_cluster=state_yt_cluster,
            run_prettify_objects=run_prettify_objects,
            solomon_endpoint=solomon_endpoint,
            juggler_endpoint=juggler_endpoint,
        )

    return cli
