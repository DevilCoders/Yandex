#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt
import telepot
from core.settings import SETTINGS

class EActions(object):
    ANNOUNCE = "announce"

    ALL = [ANNOUNCE]

def get_parser():
    parser = ArgumentParserExt(description="Create startrek task from console")
    parser.add_argument("-a", "--action", type=str, required=True,
        choices=EActions.ALL,
        help="Obligatory. Action to execute")
    parser.add_argument("-m", "--message", type=str, default=None,
        help="Optional. Message to announce (for action <%s>)" % EActions.ANNOUNCE)

    return parser

def main_announce(options):
    if not 'GENCFG_GELEGRAM_BOTTOKEN' not in os.environ:
        raise Exception('Environ variable <GENCFG_GELEGRAM_BOTTOKEN> with bot token is not defined')

    bot = telepot.Bot(os.environ['GENCFG_GELEGRAM_BOTTOKEN'])

    bot.sendMessage(SETTINGS.services.telegram.ids.gencfg_support, options.message)


def main(options):
    if options.action == EActions.ANNOUNCE:
        main_announce(options)
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
