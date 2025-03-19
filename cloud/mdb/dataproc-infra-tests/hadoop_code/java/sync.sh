#!/bin/bash

shopt -s expand_aliases
alias s3="aws --endpoint-url=https://storage.cloud-preprod.yandex.net --profile=e2e-preprod s3"

s3 ls s3://dataproc-e2e/jobs/sources/java/opencsv-4.1.jar
if [[ $? -ne 0 ]]; then
  curl -s https://repo1.maven.org/maven2/com/opencsv/opencsv/4.1/opencsv-4.1.jar | s3 cp - s3://dataproc-e2e/jobs/sources/java/opencsv-4.1.jar
fi

s3 ls s3://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar
if [[ $? -ne 0 ]]; then
  curl -s https://repo1.maven.org/maven2/com/ibm/icu/icu4j/61.1/icu4j-61.1.jar | s3 cp - s3://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar
fi

s3 ls s3://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar
if [[ $? -ne 0 ]]; then
  curl -s https://repo1.maven.org/maven2/commons-lang/commons-lang/2.6/commons-lang-2.6.jar | s3 cp - s3://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar
fi

echo "this is not a jar" | s3 cp - s3://dataproc-e2e/jobs/sources/java/invalid-jar.jar


mvn package -f dataproc-examples
s3 cp dataproc-examples/target/dataproc-examples-1.0.jar s3://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar

mvn -Pwith-main-class package -f dataproc-examples
s3 cp dataproc-examples/target/with-main-class/dataproc-examples-1.0-with-main-class.jar s3://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0-with-main-class.jar
