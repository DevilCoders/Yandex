import IPython
from typing import Any, List


class UserNamespace:
    def __init__(self, value: Any, name: str, suggest_to_user_as: str = ''):
        self.value = value
        self.name = name
        self.suggest_to_user_as = suggest_to_user_as


def start_repl(user_namespace: List[UserNamespace]):
    suggestion_tpl = 'In [{index}]: {suggest_to_user_as}'
    welcome_message_tpl = """
A lot of variables are imported and ready to use for your convenience, use locals() to find them.
You can try right now:

{suggestions}

Examples above work without any imports. To learn more about IPython, try watching
https://www.youtube.com/watch?v=3i6db5zX3Rw

"""
    suggestions = []
    index = 0
    for suggest in user_namespace:
        if suggest.suggest_to_user_as:
            index += 1
            suggestions.append(suggestion_tpl.format(index=index, suggest_to_user_as=suggest.suggest_to_user_as))
    print(welcome_message_tpl.format(suggestions='\n'.join(suggestions)))
    IPython.start_ipython(
        argv=[],
        user_ns={suggest.name: suggest.value for suggest in user_namespace},
    )
