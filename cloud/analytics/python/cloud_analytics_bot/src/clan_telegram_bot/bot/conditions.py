from clan_telegram_bot.ui.UI import Buttons
import logging

logger  = logging.getLogger(__name__)

def simple_message(func):
    def wrapper(cons):
        if cons.update.message:
            return func(cons)
        return False

    return wrapper


def callback_message(func):
    def wrapper(cons):
        if cons.update.callback_query:
            return func(cons)
        return False

    return wrapper


class Conditions:


    @staticmethod
    @simple_message
    def is_float(cons):
        try:
            float(cons.update.message.text)
            return True
        except Exception:
            return False

    @staticmethod
    @simple_message
    def is_not_empty(cons):
        res = (cons.update.message.text is not None) and len(cons.update.message.text) > 1\
             and (cons.update.message.text != Buttons.BACK_TO_MAIN)

        return res
        


    @staticmethod
    def is_subscribed(cons):
        current_user = cons.db_adapter.get_user(eff_user=cons.update.effective_user)
        result = current_user.is_subscribed == 1
        cons.db_adapter.Session.remove()
        return result

    @staticmethod
    def is_not_subscribed(cons):
        return not Conditions.is_subscribed(cons)

 
    @staticmethod
    @simple_message
    def is_get_projects_button(cons):
        return cons.update.message.text == Buttons.GET_PROJECTS

    @staticmethod
    @simple_message
    def is_back_to_main_button(cons):
        return cons.update.message.text == Buttons.BACK_TO_MAIN
    
    @staticmethod
    @simple_message
    def is_not_back_to_main_button(cons):
        return cons.update.message.text != Buttons.BACK_TO_MAIN
    
    @staticmethod
    @simple_message
    def add_project_input(cons):
        return cons.update.message.text == Buttons.ADD_PROJECT

    
    @staticmethod
    def selected_project(cons):
        # projects = cons.db_adapter.get_projects()
        # cons.db_adapter.Session.remove()

        # project_names = [project.name for project in projects]
        # project_name = cons.update.message.text 
        # res = project_name in project_names
        # user_id = cons.update.effective_user.id
        # logger.debug(f'User {user_id} selected project {project_name}')


        return cons.update.message.text != Buttons.BACK_TO_MAIN

    @staticmethod
    @simple_message
    def get_stats(cons):
        return cons.update.message.text == Buttons.GET_STATS
