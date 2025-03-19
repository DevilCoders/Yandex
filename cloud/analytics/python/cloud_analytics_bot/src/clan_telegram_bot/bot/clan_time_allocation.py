# -*- coding: utf-8 -*-
import logging
import sys

from telegram.ext import Job

from clan_telegram_bot.bot.conditions import Conditions
from clan_telegram_bot.bot.handlers import Handlers, CLANUpdater
from clan_telegram_bot.db.CLANDbAdapter import CLANDbAdapter, CLANUser
from tbc.constructor import Constructor
from clan_tools.data_adapters.TrackerAdapter import TrackerAdapter


logger = logging.getLogger(__name__)


def get_bot(bot_conf):
    try:
        bot_db_adapter = CLANDbAdapter(db_con_path=bot_conf['DB_CON'], user_class=CLANUser)

    except Exception as e:
        logger.error('Failed to connect to data. Reason {}'.format(e.message))
        sys.exit(0)

    developers_ids = bot_conf['DEVELOPERS_IDS']

    handlers = Handlers(bot_db_adapter, developers_ids, tracker_adapter=TrackerAdapter())

    cons = Constructor(token=bot_conf['TOKEN'], db_adapter=bot_db_adapter, 
                       updater=CLANUpdater(bot_conf, bot_db_adapter))
                       
    cons.add_state(name=Constructor.START_STATE_NAME)

    cons.add_state(name='is_subscribed', on_enter=handlers.not_subscribed)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source=Constructor.START_STATE_NAME,
                        dest='is_subscribed',
                        conditions=Conditions.is_not_subscribed
                        )

    cons.add_state(name='main_menu', on_enter=handlers.show_main_menu)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='is_subscribed',
                        dest='main_menu',
                        conditions=Conditions.is_subscribed)

    cons.add_state(name='main_menu', on_enter=handlers.show_main_menu)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source=Constructor.START_STATE_NAME,
                        dest='main_menu',
                        conditions=Conditions.is_subscribed)

    cons.add_job_queues([Job(handlers.subscribe_events_callback, Handlers.EVENTS_JOB_FREQ)])

    cons.add_state(name='add_project_input', on_enter=handlers.add_project_input)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='main_menu',
                        dest='add_project_input',
                        conditions=Conditions.add_project_input)

    cons.add_state(name='add_project_name', on_enter=handlers.add_project_name)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='add_project_input',
                        dest='add_project_name',
                        conditions=Conditions.is_not_empty)

    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='add_project_input',
                        dest='main_menu',
                        conditions=Conditions.is_back_to_main_button)                   

    cons.add_transition(trigger=Constructor.PASSING_TRIGGER,
                        source='add_project_name',
                        dest='main_menu')

    cons.add_state(name='get_projects_input', on_enter=handlers.get_project_input)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='main_menu',
                        dest='get_projects_input',
                        conditions=Conditions.is_get_projects_button)

    cons.add_state(name='selected_project', on_enter=handlers.input_hours)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='get_projects_input',
                        dest='selected_project',
                        conditions=Conditions.selected_project)

    cons.add_state(name='input_hours', on_enter=handlers.commit_hours)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='selected_project',
                        dest='input_hours',
                        conditions=Conditions.is_float)

    cons.add_transition(trigger=Constructor.PASSING_TRIGGER,
                        source='input_hours',
                        dest='main_menu')


    cons.add_state(name='get_stats', on_enter=handlers.get_stats)
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='main_menu',
                        dest='get_stats',
                        conditions=Conditions.get_stats)

    cons.add_transition(trigger=Constructor.PASSING_TRIGGER,
                        source='get_stats',
                        dest='main_menu')


    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='get_stats',
                        dest='main_menu',
                        conditions=Conditions.is_back_to_main_button)
    
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='selected_project',
                        dest='main_menu',
                        conditions=Conditions.is_back_to_main_button)

    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='input_hours',
                        dest='main_menu',
                        conditions=Conditions.is_back_to_main_button)
    
    cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
                        source='get_projects_input',
                        dest='main_menu',
                        conditions=Conditions.is_back_to_main_button)


    # cons.add_state(name='not_subscribed', on_enter=handlers.not_subscribed)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='main_menu',
    #                     dest='not_subscribed',
    #                     conditions=Conditions.is_not_subscribed)

    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='not_subscribed',
    #                     dest='input_severity_level',
    #                     conditions=Conditions.is_subscribed)

    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='not_subscribed',
    #                     dest='not_subscribed',
    #                     conditions=Conditions.is_not_subscribed)

    # cons.add_state(name='severity_level', on_enter=handlers.set_severity_level)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='input_severity_level',
    #                     dest='severity_level', conditions=Conditions.is_float)

    # cons.add_state(name='get_events_menu', on_enter=handlers.get_events_menu)
    # cons.add_transition(trigger=Constructor.PASSING_TRIGGER,
    #                     source='severity_level',
    #                     dest='get_events_menu')

    # cons.add_state(name='get_events', on_enter=handlers.input_hours_back)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='get_events_menu',
    #                     dest='get_events',
    #                     conditions=Conditions.is_get_events_button)

    # cons.add_state(name='get_events_inputted_hours_back', on_enter=handlers.get_events)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='get_events',
    #                     dest='get_events_inputted_hours_back',
    #                     conditions=Conditions.is_float)

    # cons.add_transition(trigger=Constructor.PASSING_TRIGGER,
    #                     source='get_events_inputted_hours_back',
    #                     dest='get_events_menu')

    # cons.add_state(name='change_severity_level', on_enter=handlers.show_main_menu)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='get_events_menu',
    #                     conditions=Conditions.is_change_severity_level_button,
    #                     dest='input_severity_level')

    # cons.add_state(name='get_stats', on_enter=handlers.input_days_back)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='get_events_menu',
    #                     dest='get_stats',
    #                     conditions=Conditions.is_get_stats_button)

    # cons.add_state(name='set_stats_days_back', on_enter=handlers.set_stats_days_back)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='get_stats',
    #                     dest='set_stats_days_back',
    #                     conditions=Conditions.is_float)

    # cons.add_state(name='get_stats_severity', on_enter=handlers.get_stats)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='set_stats_days_back',
    #                     dest='get_stats_severity')

    # cons.add_transition(trigger=Constructor.PASSING_TRIGGER,
    #                     source='get_stats_severity',
    #                     dest='get_events_menu')

    # cons.add_state(name='plot_event', on_enter=handlers.plot_event)
    # cons.add_transition(trigger=Constructor.FREE_TEXT_TRIGGER,
    #                     source='get_events_menu',
    #                     dest='plot_event',
    #                     conditions=Conditions.is_plot_event)

    # cons.add_transition(trigger=Constructor.PASSING_TRIGGER,
    #                     source='plot_event',
    #                     dest='get_events_menu')

    return cons
