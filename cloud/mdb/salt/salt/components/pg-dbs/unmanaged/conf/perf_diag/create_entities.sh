#!/bin/bash -x

all_variables_found=false
PERFDIAG_PATH="dataplane_rt/perf_diag"
declare -a TOPIC_LIST=("my_sessions" 
                       "my_statements"
                       "pg_stat_activity"
                       "pg_stat_statements"
                       )
DEFAULT_PARTITION_COUNT=1
DEFAULT_MDB_WRITER="yc.mdb.logs_dataplane_producer@as"

for i in "$@"; do
  case $i in
    -e=*|--environment=*)
      ENVIRONMENT="${i#*=}"
      all_variables_found=true
      shift # past argument=value
      ;;
    # --default)
    #   DEFAULT=YES
    #   shift # past argument with no value
    #   ;;
    -*|--*)
      echo "Unknown option $i"
      exit 1
      ;;
    *)
      ;;
  esac
done

if [ "$all_variables_found" = false  ]; then
	echo "Usage create_entities.sh -e={preprod|prod}"
	exit 1
fi

case ${ENVIRONMENT} in
  compute-preprod)
        LB_INST=yc-logbroker-preprod
	DT_AS="bfbgnr9a209qmtjepb55@as"
	LB_ACCOUNT=aoe9shbqc2v314v7fp3d
        YC_INST=compute-preprod
	YC_CH_TYPE="s2.micro"
	CH_POSTFIX="compute-preprod"
	DT_CONFIG_POSTFIX="preprod"
	METRICS_ID="bfb23d4tss48qn1eena6"
    ;;
  compute-prod)
        LB_INST=yc-logbroker
	DT_AS="ajet5egu65m66slf9tn0@as"
	LB_ACCOUNT=b1ggh9onj7ljr7m9cici
        YC_INST=compute-prod
        YC_CH_TYPE="s2.medium"
	CH_POSTFIX="compute-prod"
	DT_CONFIG_POSTFIX="prod"
	METRICS_ID="aje3hothd161ohknfmna"
    ;;
  porto-test)
        LB_INST=logbroker
        DT_AS="robot-lf-dyn-push@staff"
        LB_ACCOUNT=mdb
        PERFDIAG_PATH="porto/test_rt/perf_diag"
        DEFAULT_MDB_WRITER="2018950@tvm"
        YC_INST=porto-prod
        YC_CH_TYPE="s3.nano"
        CH_SIZE=10
        YC_ZK_TYPE="s3.nano"
        CH_POSTFIX="porto-test"
        DT_CONFIG_POSTFIX="porto"
    ;;
  porto-prod)
        LB_INST=logbroker
        DT_AS="robot-lf-dyn-push@staff"
        LB_ACCOUNT=mdb
        PERFDIAG_PATH="porto/prod_rt/perf_diag"
        DEFAULT_MDB_WRITER="2018952@tvm"
        YC_INST=porto-prod
        YC_CH_TYPE="s3.xlarge"
        CH_SIZE=1470
        YC_ZK_TYPE="s3.small"
        CH_POSTFIX="porto-prod"
        DT_CONFIG_POSTFIX="porto"
    ;;
  *)
    echo "unkonwn environemnt ${ENVIRONMENT}"
    exit 1
    ;;
esac 


create_logbroker_topics() {
	ya tool logbroker -s ${LB_INST} schema create directory ${LB_ACCOUNT}/${PERFDIAG_PATH} -y
        ya tool logbroker -s ${LB_INST} schema create consumer ${LB_ACCOUNT}/${PERFDIAG_PATH}/consumer -y
	ya tool logbroker -s ${LB_INST} permissions grant --path ${LB_ACCOUNT}/${PERFDIAG_PATH}/consumer --subject ${DT_AS} --permissions ReadAsConsumer -y
	for topic in "${TOPIC_LIST[@]}"
	do
	   echo "Will create $topic"
	   ya tool logbroker -s ${LB_INST} schema create topic ${LB_ACCOUNT}/${PERFDIAG_PATH}/${topic} -p ${DEFAULT_PARTITION_COUNT} -y
	   ya tool logbroker -s ${LB_INST} schema create read-rule -t ${LB_ACCOUNT}/${PERFDIAG_PATH}/${topic} -c ${LB_ACCOUNT}/${PERFDIAG_PATH}/consumer --all-original -y
	   ya tool logbroker -s ${LB_INST} permissions grant --path ${LB_ACCOUNT}/${PERFDIAG_PATH}/${topic} --subject ${DT_AS} --permissions ReadTopic -y
           ya tool logbroker -s ${LB_INST} permissions grant --path ${LB_ACCOUNT}/${PERFDIAG_PATH}/${topic} --subject ${DEFAULT_MDB_WRITER} --permissions WriteTopic -y
	done

}

exit_when_error() {
	if (( $1 > 0 )); then
		echo "Operation result is $1 so exit"
		exit $1
	fi
}

create_ch(){

# В будущем кластера мы будем создавать с помощью terraform https://bb.yandex-team.ru/projects/MDB/repos/controlplane/browse , а пока что используем yc

echo -n "Enter tm_writer password value : "
read tm_writer_password

case ${ENVIRONMENT} in
  compute-preprod|compute-prod)

yc --profile ${YC_INST} \
managed-clickhouse \
cluster create \
--name perfdiag-postgres-${CH_POSTFIX} \
--description "Performance diagnostick data for postgres and mysql" \
--environment production \
--network-name mdb-controlplane-dualstack-nets \
--host zone-id=ru-central1-a,subnet-name=mdb-controlplane-dualstack-nets-ru-central1-a,type=clickhouse \
--host zone-id=ru-central1-b,subnet-name=mdb-controlplane-dualstack-nets-ru-central1-b,type=clickhouse \
--host zone-id=ru-central1-c,subnet-name=mdb-controlplane-dualstack-nets-ru-central1-c,type=clickhouse \
--user name=tm_writer,password=${tm_writer_password} \
--database name=perf_diag \
--shard-name shard1 \
--version 22.3 \
--clickhouse-resource-preset ${YC_CH_TYPE} \
--clickhouse-disk-size 512 \
--clickhouse-disk-type network-ssd \
--service-account ${METRICS_ID} \
--embedded-keeper

exit_when_error $?
;;
  porto-test|porto-prod)
yc --profile ${YC_INST} \
managed-clickhouse \
cluster create \
--name perfdiag-postgres-${CH_POSTFIX} \
--description "Performance diagnostick data for postgres and mysql" \
--network-id "_MDB_CONTROLPLANE_PORTO_TEST_NETS_" \
--environment production \
--host zone-id=sas,type=clickhouse \
--host zone-id=vla,type=clickhouse \
--host zone-id=vlx,type=clickhouse \
--user name=tm_writer,password=${tm_writer_password} \
--database name=perf_diag \
--shard-name shard1 \
--version 22.3 \
--clickhouse-resource-preset ${YC_CH_TYPE} \
--clickhouse-disk-size ${CH_SIZE} \
--clickhouse-disk-type local-ssd \
--embedded-keeper

exit_when_error $?

;;
esac

yc --profile ${YC_INST} \
clickhouse \
user create \
dbaas_api_reader \
--cluster-name perfdiag-postgres-${CH_POSTFIX} \
--password ${tm_writer_password} \
--permissions perf_diag

exit_when_error $?

}

create_dt_rules(){

case ${ENVIRONMENT} in
  compute-preprod|compute-prod)
        echo "Obtain IAM token using yc: yc iam create-token"
        read -r -p "Enter token: " TOKEN
        test -z "${TOKEN}" && { echo "Need token to continue"; exit 1; }
        export TOKEN=${TOKEN}

	cd $HOME/arcadia/cloud/mdb/bootstrap/datatransfer
	for service in mysql_perf_diag postgres_perf_diag
	do
	  python3 cli.py --conf config/compute-${DT_CONFIG_POSTFIX}/${service}.yaml create
	  python3 cli.py --conf config/compute-${DT_CONFIG_POSTFIX}/${service}.yaml start
	  python3 cli.py --conf config/compute-${DT_CONFIG_POSTFIX}/${service}.yaml drop-orphans
	done
  ;;
  porto-test|porto-prod)
        echo "Obtain internal Passport token here:"
        echo "https://oauth.yandex-team.ru/authorize?response_type=token&client_id=8cdb2f6a0dca48398c6880312ee2f78d"
        read -r -p "Enter token: " TOKEN
        test -z "${TOKEN}" && { echo "Need token to continue"; exit 1; }
        export TOKEN=${TOKEN}

        cd $HOME/arcadia/cloud/mdb/bootstrap/datatransfer
        # for service in mysql_perf_diag_rt postgres_perf_diag_rt
        for service in postgres_perf_diag_rt
        do
          python3 cli.py --conf config/${DT_CONFIG_POSTFIX}/${service}.yaml create
          python3 cli.py --conf config/${DT_CONFIG_POSTFIX}/${service}.yaml start
          python3 cli.py --conf config/${DT_CONFIG_POSTFIX}/${service}.yaml drop-orphans
        done

;;
esac

}

# create_logbroker_topics
# create_ch
create_dt_rules
