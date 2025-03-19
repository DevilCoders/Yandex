import os

import pyuwsgi


def main():
    cmd = [
        '--master',
        '--lazy-apps',
        '--logformat', '[%(ltime)] %(host) %(addr) "%(method) %(uri)" %(status) "%(referer)" "%(uagent)" "-" %(msecs)',

        '--plugins-dir', '/usr/lib/uwsgi/plugins'
        '--die-on-term',
        '--no-orphans',

        '--processes', '4',
        '--module', 'cloud.marketplace.api.wsgi.wsgi',

        '--callable', 'create_app()',

        '--buffer-size', 65535,
        '--harakiri', '30',
        '--harakiri-verbose',
        '--max-requests', '1000',
        '--plugin', 'python35',
        '--http', '127.0.0.1:{}'.format(os.getenv("YC_PORT", "8000")),
        '--unique-cron', '0 -1 -1 -1 -1 find /tmp/yc_marketplace_api/prom_data -type f -name "*.db" -mmin +240 -delete',

    ]

    pyuwsgi.run(cmd)


if __name__ == '__main__':
    main()
