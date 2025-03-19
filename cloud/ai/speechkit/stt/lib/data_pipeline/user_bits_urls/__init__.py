import typing


def add_user_urls_to_tasks(tasks: typing.List[dict], user_urls_map: typing.Dict[str, str]):
    for task in tasks:
        task['input_values']['user_url'] = user_urls_map[task['input_values']['url']]
