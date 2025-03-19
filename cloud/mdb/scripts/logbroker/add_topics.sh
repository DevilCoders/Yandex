
porto_envs="test prod"
compute_envs="preprod prod"
components="${@}"

for env in ${porto_evns}
do
    for db in ${components}
    do
        echo "topic: /mdb/porto/${env}/${db}-logs"
        logbroker  -s logbroker schema create topic \
            /mdb/porto/${env}/${db}-logs \
            --abc-service internalmdb \
            --retention-period-sec 129600 \
            -p 2 \
            --responsible "mdb@" \
            -y
    done
done

for env in ${compute_envs}
do
    for db in ${components}
    do
        echo "topic: /mdb/compute/${env}/${db}-logs"
        logbroker  -s logbroker schema create topic \
            /mdb/compute/${env}/${db}-logs \
            --abc-service dbaas \
            --retention-period-sec 129600 \
            -p 2 \
            --responsible "mdb@" \
            -y
    done
done
