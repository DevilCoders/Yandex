#!/usr/bin/env python3
"""Main quotactl module."""

import os
import sys
import yaml
import logging

from datetime import datetime as dt
from yaml import load, Loader
from prompt_toolkit import prompt

from quota import QuotaService
from quota.subject import Subject
from quota.version import __version__
from quota.utils.validators import validate_service, validate_subject_id, validate_multiplier
from quota.utils.autocomplete import services_completer
from quota.constants import CONVERTABLE_VALUES, SERVICE_ALIASES, SERVICES, BAD_STATES
from quota.utils.helpers import (print_as_table, human_to_bytes, log, yaml_to_dict,
                                 Color, debug_msg, init_config, interactive_help, warning_message)
from quota.error import (ServiceError, ValidateError, ConfigError,
                         SSLError, TooManyParams, FeatureNotImplemented, ParamsError)

from cli import args, OPTIONAL_ARGS

# TODO (akimrx): create func like a warning_message for check user input (yes/no)
# TODO (akimrx): replace convertable_values to name.endswith('size')
# TODO (akimrx): check s3 patch/post method for update quota
# TODO (akimrx): quota-calc support

homedir = os.path.expanduser('~')
config_dir = os.path.join(homedir, '.config')
config_file = args.config or os.path.join(config_dir, 'qctl.yaml')


class Config:
    """This class generate config object with credentials and settings or create new config file."""
    try:
        with open(config_file, 'r') as cfgfile:
            config = load(cfgfile, Loader=Loader)
            prodkey = config['private_key-prod']
            preprodkey = config['private_key-preprod']
    except (FileNotFoundError, TypeError, KeyError):
        config = dict()
        if not args.token and not args.ssl:
            print(Color.make('Config file corrupted or not found.', color='red'))
            generate_cfg = input('Do you want to generate config file? [y/n]: ')

            if generate_cfg.lower() not in ('yes', 'y'):
                print(Color.make('Aborted', color='red'))
                raise ConfigError('Config file not found')
            else:
                init_config(config_dir, config_file)
                sys.exit(f'Config file created: {config_file}')

    BILLING_META = args.show_balance or config.get('billing_metadata', False)
    ENVS = {'preprod': args.preprod,
            'gpn': args.gpn}
    ENV = 'prod'
    for env in ENVS.keys():
        if ENVS[env]:
            ENV = env

    ENDPOINTS = config.get('endpoints', {}).get(ENV, {})

    # Credentials
    if ENV == 'gpn' and args.token:
        TOKEN = args.token
    elif ENV in ['prod', 'preprod']:
        # TOKEN = args.token or config.get('token')
        TOKEN = QuotaService.get_iam_token_grpc(account_id=config.get(f'user_account_id-{ENV}'),
                                                key_id=config.get(f'key_id-{ENV}'),
                                                private_key=config.get(f'private_key-{ENV}'), env=ENV, ssl=config.get('ssl'))

    else:
        error = '''Use --token {iam-token} to use GPN\nFederation config to YC https://clubs.at.yandex-team.ru/ycp/3101'''
        raise ParamsError(error)
    # if TOKEN is None:
    #     raise ConfigError('OAuth token is empty.')

    if ENV != 'gpn':
        SSL = args.ssl or config.get('ssl')
    else:
        SSL = args.ssl or config.get(f'ssl{ENV}')
    if SSL is None:
        raise SSLError('Path to root certificate is empty.')

    # if ENV == 'preprod':
    #     TOKEN = QuotaService.get_iam_token(TOKEN, env=ENV, ssl=SSL)  # ToDo: get iam via grpc here

    # Logs
    LOGLEVEL = 'debug' if args.debug else 'warning'
    if LOGLEVEL == 'debug':
        import http.client as http_client
        http_client.HTTPConnection.debuglevel = 1
    LOGLEVEL = getattr(logging, LOGLEVEL.upper())
    if not isinstance(LOGLEVEL, int):
        raise ValueError('loglevel must be: debug/info/warning/error')


log_handlers = [logging.StreamHandler()]
logging.basicConfig(level=Config.LOGLEVEL,
                    handlers=log_handlers,
                    datefmt='%H:%M:%S',
                    format='%(asctime)s %(message)s')

logger = logging.getLogger(__name__)
logger.debug(f'version: {__version__}; current time: {dt.today()}')
debug_msg()  # system message with info about OS, etc


# CLI Commands

@log
def _init_service(service: str) -> QuotaService:
    """Return service object with methods."""
    endpoint = Config.ENDPOINTS.get(service, None)

    if len(Config.TOKEN) > 100:

        quota_service = QuotaService(iam_token=Config.TOKEN, env=Config.ENV, endpoint=endpoint,
            service=service, ssl=Config.SSL)

    else:
        quota_service = QuotaService(oauth_token=Config.TOKEN, env=Config.ENV, endpoint=endpoint,
            service=service, ssl=Config.SSL)

    return quota_service


def get_subject(service: str, subject_id: str) -> Subject:
    """Get service quota metrics as subject and return."""
    validate_subject_id(subject_id)
    quota = _init_service(service)
    subject = quota.get_metrics(subject_id)
    return subject


def get_billing_metadata(subject_id: str) -> None:
    """Print billing metadata for cloud_id."""
    if not Config.BILLING_META:
        return
    billing = _init_service('billing')
    print()  # separator

    try:
        for key, v in billing.metadata(subject_id).to_dict().items():
            if isinstance(v, (int, float)) and v <= 0:
                value = Color.make(str(v), 'red')
            elif isinstance(v, str) and v.lower() in BAD_STATES:
                value = Color.make(v, 'red')
            else:
                value = v if v != 'company' else Color.make(v, 'cyan')

            print(f'{key.title()}: {value}')
    except Exception as err:
        logger.debug(err)
        print(Color.make(f"Can't parse billing metadata for {subject_id}", color='red'))


def update_concrete(service: str, subject_id: str, metric: str, value: [str, int]) -> None:
    """Update specified service quota metric."""
    validate_subject_id(subject_id)
    quota = _init_service(service)
    valid_value = human_to_bytes(value) if metric in CONVERTABLE_VALUES else int(value)

    try:
        quota.update_metric(subject_id, metric, valid_value)
        print(Color.make(f'{metric} set to {value} - OK', color='green'))
    except Exception as err:
        c=1
        print(Color.make(err, color='red'))


def update_from_yaml(filename, subject=None) -> None:
    data = yaml_to_dict(filename)
    service = _init_service(validate_service(data.get('service')))
    subject_id = validate_subject_id(subject or data.get('cloud_id'))
    metrics = data.get('metrics')
    ends = ('b', 'k', 'm', 'g', 't')

    print(f'Updating {service.service_name.title()} quota metrics for "{subject_id}"...')
    for metric in metrics:
        if metric.get('name') is None or metric.get('limit') is None:
            raise ValidateError('Metrics cannot be None.')

        name = metric.get('name')
        limit = metric.get('limit')
        limit = human_to_bytes(limit) if name in CONVERTABLE_VALUES and str(limit).lower().endswith(ends) else int(limit)

        try:
            service.update_metric(subject_id, name, limit)
            print(Color.make(f'{name} set to {limit} - OK', color='green'))
        except Exception as err:
            print(Color.make(err, color='red'))

    print_as_table(service.get_metrics(subject_id))


def update_interactive(subject: object) -> None:
    """Update service quota metrics for subject in interactive mode."""
    for metric in subject.metrics:
        awaiting_new_value = True

        while awaiting_new_value:
            try:
                new_value = input(f'{metric.name} [{metric.human_size_usage}/{metric.human_size_limit}]: ')
                if new_value in ('', ' ', 'skip', None):
                    awaiting_new_value = False
                elif new_value in ('quit', 'q', 'exit', 'abort'):
                    print(Color.make('\nAborted by "quit" command', color='yellow'))
                    return
                else:
                    multiplier = validate_multiplier(new_value)
                    if multiplier:
                        valid_value = metric.limit * multiplier
                    else:
                        valid_value = human_to_bytes(new_value) if metric.name in CONVERTABLE_VALUES else int(new_value)
                    if metric.update(valid_value):
                        print(Color.make(f'{metric.name} set to {new_value} - OK', color='green'))
                        awaiting_new_value = False
            except KeyboardInterrupt:
                print(Color.make('\nAborted by SIGINT (Ctrl+C)', color='yellow'))
                return
            except ValueError:
                print(Color.make('Invalid value. Limit must be int or multiplier, example: 10, x2', color='red'))
            except Exception as e:
                print(Color.make(e, color='red'))


def interactive_main(service_name=None, subject_id=None) -> None:
    """Launches an interactive mode where user can select a service."""
    env = Color.make(Config.ENV.upper(), 'green') if Config.ENV == 'prod' else Color.make(Config.ENV.upper(), 'yellow')
    print(f'Environment: {env}')
    await_service, await_subject = True, True

    while await_service:
        try:
            service = validate_service(service_name or prompt('Enter service name: ',
                completer=services_completer, complete_while_typing=True).strip())
            await_service = False
        except ServiceError as err:
            print(Color.make(f'\n{err}', color='red'))
            print(f'Supported services: {", ".join(SERVICES)}')
            print(f'Aliases: {", ".join(SERVICE_ALIASES)}\n')
        except KeyboardInterrupt:
            sys.exit(Color.make('\nAborted by SIGINT (Ctrl+C)', color='yellow'))


    while await_subject:
        try:
            subject_name = 'Folder' if service == 'resource-manager' else 'Cloud'
            _id = validate_subject_id(subject_id or input(f'Enter {subject_name} ID: '))
            await_subject = False
        except ValidateError as err:
            print(Color.make(err, color='red'))
        except KeyboardInterrupt:
            sys.exit(Color.make('\nAborted by SIGINT (Ctrl+C)', color='yellow'))

    if subject_name != 'Folder': get_billing_metadata(_id)

    interactive_help()

    subject = get_subject(service, _id)
    update_interactive(subject)
    print_as_table(subject.refresh())


def multiply_metrics(service: str, subject_id: str, multiplier: int) -> None:
    """Multiplies each service quota metric by a multiplier."""
    subject = get_subject(service, subject_id)
    for metric in subject.metrics:
        try:
            metric.update(metric.limit * int(multiplier))
            print(Color.make(f'{metric.name} set to x{multiplier} - OK', color='green'))
        except Exception as err:
            print(Color.make(err, color='red'))

    print_as_table(subject.refresh())


def mass_metrics_change(subject_id: str, action: str) -> None:
    """Set all service quota metrics to zero/default for subject."""
    validate_subject_id(subject_id)
    print(f'Preparing a mass change of metrics to {action} for the cloud "{subject_id}"...')
    services = args.services.split(',') if args.services else None or SERVICES

    _affected = f'services: \n{services}' if isinstance(services, list) else 'ALL SUPPORTED SERVICES!'
    warning_message(f'WARNING! This command set to {action} all quotas for {_affected}')

    for service in services:
        service = validate_service(service)

        if service in ('resource-manager', 'billing', 'compute-old'):
            continue

        s = _init_service(service)
        service_action = s.zeroize_metrics if action == 'zero' else s.reset_metrics_to_default
        print(f'Working with the {service} service quotas...')

        for i in service_action(subject=subject_id):
            print(Color.make(i, 'green') if i.endswith('- OK') else Color.make(i, 'red'))

        try:
            print_as_table(s.get_metrics(subject_id).metrics)
        except Exception as err:
            print(Color.make(err, color='red'))


def quotactl():
    logger.debug(args)
    normalize = lambda name: name.replace('_', '-')
    get_subject_id = lambda key: getattr(args, key.replace('-', '_'))

    _services = [normalize(arg) for arg in vars(args) if getattr(args, arg) and normalize(arg) in SERVICES]
    service_ctx = None if not _services else _services[0]
    logger.debug(f'services: {_services}, received service: {service_ctx}')

    if args.code:
        raise FeatureNotImplemented('Integration with the calculator is not yet supported.')

    if args.set_from_yaml:
        return update_from_yaml(args.set_from_yaml, subject=args.subject)

    if len(_services) > 1:
        raise TooManyParams(f'More that one service received: {_services}. Please specify one service.')

    if args.default:
        return mass_metrics_change(args.default, action='default')
    elif args.zeroize:
        return mass_metrics_change(args.zeroize, action='zero')

    if not _services:

        return interactive_main()
    elif args.multiply:
        return multiply_metrics(service_ctx, get_subject_id(service_ctx), multiplier=args.multiply)
    elif args.set:  # interactive or concrete metrics update for service
        if args.name and args.limit:
            update_concrete(service_ctx, get_subject_id(service_ctx), args.name, args.limit)
            print_as_table(get_subject(service_ctx, get_subject_id(service_ctx)))
            return
        else:
            return interactive_main(service_name=service_ctx, subject_id=get_subject_id(service_ctx))
    else:  # print metrics for service
        subject = get_subject(service_ctx, get_subject_id(service_ctx))
        print_as_table(subject)
        return


def main():
    try:
        quotactl()
    except Exception as error:
        sys.exit(Color.make(f'[{type(error).__name__}] {error}', color='red'))


if __name__ == '__main__':
    main()
