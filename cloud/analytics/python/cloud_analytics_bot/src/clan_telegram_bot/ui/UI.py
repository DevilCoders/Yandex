# -*- coding: utf-8 -*-
from typing import List
from telegram import ReplyKeyboardMarkup, InlineKeyboardMarkup, InlineKeyboardButton
from clan_tools.utils.time import utcms2datetime
import numpy as np

class Buttons:

    CHANGE_SEVERITY_LEVEL = 'Change alerts level'
    SET_SEVERITY_LEVEL = 'Set alerts level'
    UNSUBSCRIBE = 'Unsubscribe'
    GET_PROJECTS = 'Get projects'
    ADD_PROJECT = 'Add project'
    GET_STATS = 'Get stats'
    BACK_TO_MAIN = 'Back to Main Menu'


class InlineButtons:
    def __init__(self):
        pass

    EVENT = '⬆ Example'


class Keyboards:
    def __init__(self):
        pass

    MAIN = ReplyKeyboardMarkup(
        [[Buttons.GET_PROJECTS], [Buttons.ADD_PROJECT], [Buttons.GET_STATS]], resize_keyboard=True)
    HOURS = ReplyKeyboardMarkup([['1', '2', '3'],
                                 ['4', '5', '6'],
                                 ['7', '8', '9'],
                                 [Buttons.BACK_TO_MAIN]], resize_keyboard=True)
    BACK_TO_MAIN = ReplyKeyboardMarkup([[Buttons.BACK_TO_MAIN]], resize_keyboard=True)
    
    @staticmethod
    def plain_keyboard(items: List['str']):
        return ReplyKeyboardMarkup(np.array([items]).reshape(-1,1).tolist()
         + [[Buttons.BACK_TO_MAIN]], resize_keyboard=True)
        



class InlineKeyboards:
    def __init__(self):
        pass

    EVENT = InlineKeyboardMarkup(
        [[InlineKeyboardButton(InlineButtons.EVENT, callback_data='example')]])
