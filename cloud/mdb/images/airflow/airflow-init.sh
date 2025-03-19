#!/usr/bin/env bash

# This script is being run by airflow-init.service during instance system boot

# This initialization should be made only once
if [ ! -f /etc/airflow/secrets.txt ]; then
    # Generates random fernet key to encrypt secrets in Ariflow metadata DB
    # https://airflow.readthedocs.io/en/stable/howto/secure-connections.html
    FERNET_KEY=$(python3 -c 'from cryptography.fernet import Fernet; print(Fernet.generate_key().decode())')
    FLASK_SECRET_KEY=$(python3 -c 'from cryptography.fernet import Fernet; print(Fernet.generate_key().decode())')
    touch /etc/airflow/secrets.txt && chmod 0660 /etc/airflow/secrets.txt
    echo "AIRFLOW__CORE__FERNET_KEY=$FERNET_KEY" > /etc/airflow/secrets.txt
    echo "AIRFLOW__WEBSERVER__SECRET_KEY=$FLASK_SECRET_KEY" >> /etc/airflow/secrets.txt
    export AIRFLOW__CORE__FERNET_KEY=$FERNET_KEY
    export AIRFLOW__WEBSERVER__SECRET_KEY=$FLASK_SECRET_KEY
    export AIRFLOW_HOME=/etc/airflow

    AIRFLOW_VERSION=$(pip3 show apache-airflow | grep -i version | cut -d' ' -f2)

    # Initialize Airflow metadata DB: create tables and stuff
    airflow db init
    INSTANCE_ID=`curl --silent -H "Metadata-Flavor: Google" "http://169.254.169.254/computeMetadata/v1/instance/id"`
    airflow users create --email test@example.com --firstname test --lastname admin \
                         --username test_admin --password $INSTANCE_ID --role Admin

    echo $AIRFLOW_VERSION > /etc/airflow/version.txt
fi
