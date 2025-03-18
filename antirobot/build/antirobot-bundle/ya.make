UNION()

OWNER(g:antirobot)

BUNDLE(
    antirobot/daemon NAME antirobot_daemon
    antirobot/scripts/gencfg NAME antirobot_gencfg
    antirobot/scripts/send_mail NAME antirobot_sendmail
    antirobot/tools/evlogdump NAME antirobot_evlogdump
    antirobot/tools/json_config_checker
    antirobot/tools/ammo_generator
)

NEED_CHECK()

END()
