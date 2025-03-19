#!/usr/bin/env python
# -*-coding: utf-8 -*-

import settings

import logging
import subprocess
import json
import sys
import time


logger = logging.getLogger('log')

def log_init(log_file, log_level=logging.DEBUG):
	"""
	Function for init log file
	"""
	# Log Handler. Set logfile
	handler = logging.FileHandler(log_file)
	# create formatter for log
	formatter = logging.Formatter(\
		'%(asctime)s %(levelname)-4s %(message)s')
	# add formatter to handler
	handler.setFormatter(formatter)
	logger.addHandler(handler)
	# Set log level from config
	logger.setLevel(log_level)

def execute(commands):
	for cmd in commands:
		subprocess.call(cmd, shell=True, stdout=sys.stdout, stderr=sys.stderr)

def get_diagnose():
	cmd = "barman diagnose"
	data = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE).communicate()[0]
	return json.loads(data)

def get_last_backup(barman_data):
	backups = barman_data["servers"]["weather"]["backups"].keys()
	backups.sort(reverse=True)
	return backups[0]

def prepare_servers():
	servers = settings.PG_SLAVES
	servers.append(settings.PG_MASTER)
	for pg in servers:
		cmd = []
		cmd.append("ssh postgres@"+pg+" pg_ctlcluster "+settings.PG_VERSION_CLUSTER+" stop -m fast")
		cmd.append("ssh postgres@"+pg+" rm -r "+settings.PG_DATADIR)
		execute(cmd)

def restore_master(backup_id):
	cmd = []
	cmd.append("barman recover weather "+backup_id+" "+settings.PG_DATADIR+" --remote-ssh-command \"ssh postgres@"+settings.PG_MASTER+"\"")
	cmd.append("ssh postgres@"+settings.PG_MASTER+" pg_ctlcluster "+settings.PG_VERSION_CLUSTER+" start")
	execute(cmd)

def restore_slaves():
	cmd = []
	is_master_ready = subprocess.call("ssh postgres@"+settings.PG_MASTER+" pg_isready", shell=True, stdout=sys.stdout, stderr=sys.stderr)
	while is_master_ready > 0:
		time.sleep(10)
		is_master_ready = subprocess.call("ssh postgres@"+settings.PG_MASTER+" pg_isready", shell=True, stdout=sys.stdout, stderr=sys.stderr)

	for pg in settings.PG_SLAVES:
		cmd.append("ssh postgres@"+pg+" pg_basebackup -v -D "+settings.PG_DATADIR+" -h "+settings.PG_MASTER+" --username=repl -P -x -R")
		cmd.append("ssh postgres@"+pg+" pg_ctlcluster "+settings.PG_VERSION_CLUSTER+" start")
		execute(cmd)

def postrestore():
	for host in settings.postrestore_cmd:
		for operation in settings.postrestore_cmd[host]:
			cmd = ["ssh "+operation["user"]+"@"+host+" '"+operation["cmd"]+"'"]
			execute(cmd)
