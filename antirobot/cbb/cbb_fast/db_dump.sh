HOST_CBB=man-kcnfkahmozpmzcyx.db.yandex.net
HOST_CBB_HISTORY=man-yn9cpq1vkumb9qga.db.yandex.net

TEST_HOST_CBB=sas-rhn5lnd2e1jhzofu.db.yandex.net
TEST_HOST_CBB_HISTORY=sas-nf12y8taz13hy4hv.db.yandex.net

dump() {

        #--format custom \
        #--blobs \
    echo "enter password cbb_admin: prod"
    pg_dump \
        --dbname cbb \
        --host $HOST_CBB \
        --port 6432 \
        --username cbb_admin \
        --password \
        --clean \
        --file=cbb.sql

    echo "enter password cbb_admin: prod"
    pg_dump \
        --dbname cbb_history \
        --host $HOST_CBB_HISTORY \
        --port 6432 \
        --username cbb_admin \
        --password \
        --clean \
        --file=cbb_history.sql
}

restore() {
    echo "enter password cbb_admin: testing"
    pg_restore \
        --dbname cbb \
        --host $TEST_HOST_CBB \
        --port 6432 \
        --username cbb_user \
        --password \
        --clean \
        cbb.backup

    echo "enter password cbb_admin: testing"
    pg_restore \
        --dbname cbb_history \
        --host $TEST_HOST_CBB \
        --port 6432 \
        --username cbb_user \
        --password \
        --clean \
        cbb_history.backup
}


dump
#restore
