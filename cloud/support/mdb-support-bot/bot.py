import logging
from datetime import datetime, time, timedelta, timezone
from time import sleep

from telegram.error import BadRequest, Unauthorized
from telegram.ext import CommandHandler, Updater
from telegram.ext.dispatcher import run_async

from app.utils.classes import Duty
from app.utils.config import Config

class MDBSupportBot():
    def __init__(self):
        # Telegram updater and dispatcher
        self.token = Config.tg_prod
        self.updater = Updater(token=self.token, workers=16, use_context=False)
        self.job_worker = self.updater.job_queue
        self.dispatcher = self.updater.dispatcher
        self.queue = self.updater.job_queue

        # Reply and Duty init
        self.today_duty = Duty()

        # Commands startup
        self.dispatcher.add_handler(CommandHandler(["duty", "вген", "дюти"],
                                        self.command_duty,
                                        pass_args=True))

        # Jobs startup
        self.job_worker.run_repeating(self.job_update, 300, 0)

    def start(self):
        logging.info("[bot.start] For base!")
        self.updater.start_polling()

    def duty_team(self, input_arg):
        input_arg = input_arg.replace(',', '')
        default = self.today_duty.result_dict.get(input_arg, None)
        if default == None:
            for team in self.today_duty.result_dict.keys():
                custom_dict = self.today_duty.duty_teams.get(team, [team])
                
                if custom_dict != [team]:
                    custom_dict = custom_dict.get("custom_dict")
                    custom_dict = custom_dict.split(",")
                    custom_dict.append(team)
                for alias in custom_dict:
                    if input_arg in alias:
                        return team
            return None
        else:
            return input_arg
    
    @run_async
    def command_duty(self, bot, update, args):
        logging.info(f"[duty] chat:{update.message.chat_id}")
        if update.message.chat_id == -1001088650226 or update.message.chat_id == 126910519:
            if args:
                logging.info(f"[duty] args: {args}")
                if args[0] == 'mdb':
                    return
                target_team = self.duty_team(args[0].lower())
                if args[0] == 'clickhouse' or args[0] == 'ch':
                    target_team = self.duty_team('mdb-clickhouse')
                if target_team == None:
                    return
                logging.info(f"[bot.command_duty] {target_team} called")
                reply = self.today_duty.result_dict.get(target_team)
                update.message.reply_text(reply,
                                          parse_mode="HTML",
                                          disable_web_page_preview=True)
            else:
                update.message.reply_text(self.today_duty.result,
                                          parse_mode="HTML",
                                          disable_web_page_preview=True)
        else:
            logging.info("[bot.command_duty] Chat wasnt activated")

    def command_update(self, bot, update):
        logging.info(f"[bot.command_update] {user.login_staff} tried to use update")
        if update.message.chat_id == '126910519':
            update.message.reply_text(f"Cache refresh started")
            self.job_update(bot, update)

    def job_update(self, bot, update):
        self.today_duty.update_duty()


def main():
    logging.basicConfig(format="[%(asctime)s] %(message)s",
                        level=logging.INFO,
                        datefmt='%D %H:%M:%S')
    Gnome_Clockwork = MDBSupportBot()
    Gnome_Clockwork.start()


if __name__ == "__main__":
    main()
