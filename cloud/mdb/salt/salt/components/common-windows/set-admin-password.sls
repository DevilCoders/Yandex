administrator-password-set:
    module.run:
        - name: mdb_windows.set_administrator_password
        - m_user: '.\\Administrator'

salt-minion-restarted:
    module.run:
        - name: service.restart
        - m_name: salt-minion
