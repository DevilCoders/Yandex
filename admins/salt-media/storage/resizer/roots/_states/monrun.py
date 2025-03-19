#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
Mange monrun configuration
==========================

Control the yandex monitoring helper.
Creates/deletes monrunt config and regenerates Snaked tasks.
Only 'command' and 'name' are mandatory.
By default 'file' is '/etc/monrun/conf.d/' + name + '.conf'

.. code-block:: yaml

  mysql_slave:
      monrun.present:
            - command: /usr/bin/check_mysql_slave
            - file: /etc/monrun/conf.d/mysql_slave.conf
            - execution_interval: 60
            - execution_timeout: 30
            - any_name: any_value

  "Remove obsolete monrun file":
    monrun.absent:
        - file: /etc/monrun/conf.d/mysql_slave.conf


'''

# Import python libs
import os
import difflib
import logging
import ConfigParser

log = logging.getLogger(__name__)

def present(name,
            command,
            file=None,
            regenerate_tasks=True,
            **kwargs):

    monitoring_conf=kwargs

    default_execution_interval=60
    default_execution_timeout=30

    if not monitoring_conf.has_key('execution_interval'):
        monitoring_conf['execution_interval']=default_execution_interval
    if not monitoring_conf.has_key('execution_timeout'):
        monitoring_conf['execution_timeout']=default_execution_timeout
    
    monitoring_conf['command']=command

    if file is None:
        file="/etc/monrun/conf.d/{0}.conf".format(name)

    ret = {'name': name,
           'changes': {},
           'result': True,
           'comment': ''}
    
    old=''
    changed=False

    monrun_config = ConfigParser.ConfigParser()

    if os.path.isfile(file):
        # Читаем старый конфиг (чтобы показать diff)
        with open(file, 'r') as old_cf_fh:
            old=old_cf_fh.readlines()

        monrun_config.read(file)
        if monrun_config.has_section(name):
            for item in monitoring_conf:
                new_val     = str(monitoring_conf[item])
                try:
                    config_val  = str(monrun_config.get(name,item))
                except ConfigParser.NoOptionError:
                    config_val  = ''
    
                if config_val != new_val:
                    monrun_config.set(name, item, new_val)
                    changed=True

    if not monrun_config.has_section(name):
        monrun_config.add_section(name)
        changed=True
        for item in monitoring_conf:
            monrun_config.set(name, item, monitoring_conf[item])
    
    if changed == True:
        with open(file, 'wb') as cf_fh:
            monrun_config.write(cf_fh)
        with open(file, 'r') as new_cf_fh:
            new=new_cf_fh.readlines()

        ret['changes']['diff'] = ''.join(difflib.unified_diff(old,new))
        ret['comment'] = ret['comment'] + 'Monrun config {0} updated.'.format(file)

        # Сгенерируем новые snaked-джобы
        # FIXME: стейт сфейлится только в первый раз!
        if regenerate_tasks==True:
            ret['result'] = _regenerate_tasks()
            if ret['result'] == False:
                ret['comment'] = ret['comment'] + "\nFailed to regenerate monrun tasks!"
    else:
        ret['comment'] = 'Monrun config  {0} is in the correct state'.format(file)

    return ret

def absent( name,
            file=None,
            regenerate_tasks=True,
            **kwargs ):

    import salt.utils
    import salt.states.file
    
    salt.states.file.__salt__   = __salt__
    salt.states.file.__opts__   = __opts__
    salt.states.file.__grains__ = __grains__
    salt.states.file.__pillar__ = __pillar__
    salt.states.file.__env__    = __env__

    if file is None:
        file="/etc/monrun/conf.d/{0}.conf".format(name)

    ret = salt.states.file.absent(file)
    if ret['changes']:
        if regenerate_tasks==True:
            ret['result'] = _regenerate_tasks()
            if ret['result'] == False:
                ret['comment'] = ret['comment'] + "\nFailed to regenerate monrun tasks!"
    return ret

def _regenerate_tasks():
    regenerate_bin='/usr/sbin/regenerate-monrun-tasks >/dev/null 2>&1'

    result = os.system(regenerate_bin)
    if result == 0:
        log.info("Regenerating snaked jobs succeeded.")
        return True
    else:
        log.info("Regenerating snaked jobs failed.")
        return False

