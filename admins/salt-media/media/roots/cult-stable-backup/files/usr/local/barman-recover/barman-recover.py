#!/usr/bin/env python
# -*-coding: utf-8 -*-

import sys, argparse, imp

argparser = argparse.ArgumentParser(description='Automatic recover PG backup')
argparser.add_argument('--config', required=True)

config_path = argparser.parse_args().config

settings = imp.load_source('settings', config_path)

# Импортируем функции после импорта settings, чтобы настройки подхватились
from utilities import logger, log_init, get_diagnose, get_last_backup, prepare_servers, restore_master, restore_slaves, postrestore

def main():
	logger.info("Recover started")
	logger.info("PG master - %s", settings.PG_MASTER)

	barman_data = get_diagnose()
	backup_date = get_last_backup(barman_data)
	logger.info("Using backup %s", backup_date)

	prepare_servers()
	logger.info("PostgreSQL stopped, data removed")
	restore_master(backup_date)
	logger.info("Master recovered")
	restore_slaves()
	logger.info("Slave recovered")
	logger.info("Recover completed")

	try:
		logger.info("Trying postrestore")
		postrestore()
		logger.info("Postrestore completed")
	except:
		logger.info("Postrestore failed")
		

if __name__ == "__main__":
	log_init(settings.logfile)
	main()
