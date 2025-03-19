"""
Command-line prompts.
"""

import os

import click


class PromptChoice:
    def __init__(self, number, value):
        self.number = number
        self.value = value


def confirm_dangerous_action(message, suppress):
    if not suppress:
        if not click.confirm(f'Warning: {message} Proceed?', default=False):
            exit(0)


def numeric_prompt(options, *, default=None):
    return_values = []
    for i, option in enumerate(options):
        if isinstance(option, tuple):
            name = option[0]
            return_value = option[1]
        else:
            name = option
            return_value = option

        if return_value == default:
            default = i + 1

        click.echo(f' [{i + 1}] {name}')
        return_values.append(return_value)

    value = click.prompt('Please enter your numeric choice', default=default)
    while True:
        try:
            int_value = int(value)
            if int_value >= 1 and int_value <= len(options):
                return PromptChoice(int_value, return_values[int_value - 1])
        except Exception:
            pass

        value = click.prompt(f'Please enter a value between 1 and {len(options)}')


def path_prompt(message, suggestions=None, default=None):
    if suggestions is None:
        suggestions = []

    if suggestions:
        click.echo(f'{message}:')
        for i, suggestion in enumerate(suggestions):
            click.echo(f' [{i + 1}] {suggestion}')
        value = click.prompt('Please specify numeric choice or file path', default=default)
    else:
        value = click.prompt(message, default=default)

    if value == default:
        return value

    while True:
        try:
            suggestion_num = int(value)
            if suggestion_num >= 1 and suggestion_num <= len(suggestions):
                return suggestions[suggestion_num - 1]
        except Exception:
            pass

        try:
            value = os.path.expanduser(value)
            if os.path.exists(value):
                return value
        except Exception:
            pass

        if suggestions:
            value = click.prompt(f'Please specify a value between 1 and {len(suggestions)}, or valid file path')
        else:
            value = click.prompt('Please specify a valid file path')


def prompt(message, *, default=None, hide_input=False, blacklist=None, blacklist_message=None, retry_message=None):
    if retry_message is None:
        retry_message = message
    if blacklist is None:
        blacklist = []

    value = click.prompt(message, default=default, hide_input=hide_input)
    while True:
        if value in blacklist:
            click.echo(blacklist_message.format(value=value))
            value = click.prompt(retry_message, hide_input=hide_input)
            continue

        return value
