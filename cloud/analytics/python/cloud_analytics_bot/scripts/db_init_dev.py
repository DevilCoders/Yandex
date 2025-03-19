from clan_telegram_bot.db.CLANDbAdapter import CLANDbAdapter, CLANUser
from clan_tools.utils.conf import read_conf

conf = read_conf('config/clan_telegram_bot_dev.conf')

if __name__ == '__main__':
    adapter = CLANDbAdapter(db_con_path=conf['DB_CON'], user_class=CLANUser)
    adapter.init_db()
