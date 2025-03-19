"""Console UI helper functions"""

import sys


def _prompt(question: str) -> str:
    sys.stdout.write(question)
    return input()


def ask_confirmation(question: str, yes_answer: str = "yes", no_answer: str = "no") -> bool:
    """Ask a yes/no question via input() and return their answer."""
    question = "{} [{}/{}]: ".format(question, yes_answer, no_answer)
    while True:
        answer = _prompt(question)
        if answer == yes_answer:
            return True
        elif answer == no_answer:
            return False
        else:
            print("Please, respond with '{}' or '{}'".format(yes_answer, no_answer))
