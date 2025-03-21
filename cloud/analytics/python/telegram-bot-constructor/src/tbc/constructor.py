import logging
import re
import traceback

from telegram.ext import Updater, MessageHandler, Filters, CallbackQueryHandler
from transitions import Machine, State

logger = logging.getLogger(__name__)


class Constructor:
    """
    Main bot constructor class, which contains many operational fields: bot (telegram), update (telegram), 
    state-machine fields, db_adapter for handling users' states and so on
    """
    START_STATE_NAME = '__start__'
    FREE_TEXT_TRIGGER = '__free_text__'
    PASSING_TRIGGER = '__passing_trigger__'

    def __init__(self, token, db_adapter, updater=None):
        """
        :param token: toke of your bot (could be obtained within @BotFather in telegram).
        :param db_adapter: object of the original class DbAdapter (db_adapter.DbAdapter) or object of the inherited
            class from DbAdapter

        :type token: str
        :type db_adapter: DbAdapter
        """
        self.token = token
        self.update = None
        self.state = None
        self.machine = None
        self.start_state = None
        self.states = []
        self.transitions = []
        self.db_adapter = db_adapter
        self.dp = None
        self.jobs = []
        self.bot = None
        self.updater = updater

    def __handler(self, bot, update, trigger):
        """
        Handler method which is activated when bot receives message (or another type of input) from user. This method
            deals with self.db_adapter: takes the current user's state from db -> set this state to state machine ->
            execute the related machine handler (with given state and trigger) -> update state in self.state and
            update user's state in db
        """
        eff_user = update.effective_user
        user_id = eff_user.id
        logger.info('Handling user with id: {}'.format(user_id))
        self.user = self.db_adapter.get_user(eff_user=eff_user)
        self.bot = bot
        self.update = update
        self.user.state = Constructor.START_STATE_NAME if not self.user.state else self.user.state

        if self.machine:
            self.machine.set_state(state=self.user.state, model=self)
        else:
            self.machine = Machine(model=self, states=self.states, initial=self.user.state,
                                   transitions=self.transitions)

        triggers = self.machine.get_triggers(self.state)
        matched_triggers = []
        for possible_trigger in triggers:
            if re.match(possible_trigger, trigger):
                matched_triggers.append(possible_trigger)

        if len(matched_triggers) == 0:
            trigger = Constructor.FREE_TEXT_TRIGGER
        elif len(matched_triggers) == 1:
            trigger = matched_triggers[0]
        else:
            raise ValueError(
                'Proposed trigger {} has more than one possible model\'s matched triggers: {}'.format(trigger,
                                                                                                      matched_triggers))

        self.machine.model.trigger(trigger, self)

        self.user.state = self.state
        self.db_adapter.commit_user(self.user)

        if Constructor.PASSING_TRIGGER in self.machine.get_triggers(self.state):
            self.__handler(self, update, Constructor.PASSING_TRIGGER)

    def __msg_handler(self, bot, update):
        """
        Executes self.__handler if bot receives text from user
        """
        trigger = update.message.text
        try:
            self.__handler(bot, update, trigger)
        except Exception as e:
            __msg_handler_err_msg = 'Failed to handle message. Reason {}'.format(e)
            # update.message.reply_text(__msg_handler_err_msg)
            logger.error(__msg_handler_err_msg)
            logger.error(traceback.format_exc())

    def __clb_handler(self, bot, update):
        """
        Executes self.__handler if bot receives callback data from user
        """
        trigger = update.callback_query.data
        try:
            self.__handler(bot, update, trigger)
        except Exception as e:
            __clb_handler_err_msg = 'Failed to handle message. Reason {}'.format(e)
            logger.error(__clb_handler_err_msg)
            logger.error(traceback.format_exc())


    def add_state(self, name, on_enter=None, on_exit=None):
        """
        Append new state to the bot's states list
        See transitions.State class docs for more details.
        """
        args = locals()
        del args['self']
        self.states.append(State(**args))

    def add_transition(self, trigger, source, dest, conditions=None, unless=None, before=None,
                       after=None, prepare=None):
        """
        Append new transitions to the bot's transitions list.
        See transitions.Transition class docs for more details.
        """
        args = locals()
        del args['self']
        self.transitions += [args]

    def add_job_queues(self, jobs):
        self.jobs = jobs

    def __add_job_queues(self, jobs):
        for j in jobs:
            self.dp.job_queue.run_repeating(j.callback, interval=j.interval, first=0)

    def main(self, start_message=None):
        """
        Execute this method when bot is ready
        """

        if not self.updater:
            self.updater = Updater(self.token)

        self.dp = self.updater.dispatcher

        self.dp.add_handler(MessageHandler(Filters.text, self.__msg_handler))
        self.dp.add_handler(MessageHandler(Filters.command, self.__msg_handler))
        self.dp.add_handler(CallbackQueryHandler(callback=self.__clb_handler))

        self.__add_job_queues(self.jobs)
        self.updater.start_polling()
        self.updater.idle()
