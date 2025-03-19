import logging.config
import os
import click
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
from pyspark.sql.functions import col
from spyt import spark_session
from datetime import datetime
import pymorphy2
import re

ma = pymorphy2.MorphAnalyzer()


def num_digits(s):
    return sum(c.isdigit() for c in s)


def clean_text(text, words_count=100):
    text = text.replace("\\", " ")
    text = text.lower()
    text = re.sub('\-\s\r\n\s{1,}|\-\s\r\n|\r\n', ' ', text)
    text = re.sub(
        '[.,:;_%©?*,!@#$%^&(){{}}]|[+=]|[«»]|[<>]|[\']|[[]|[]]|[/]|"|\s{2,}|-', ' ', text)
    text = ' '.join(word for word in text.split() if len(word) > 2)
    text = ' '.join(word for word in text.split() if not word.isnumeric())
    text = ' '.join(word for word in text.split() if num_digits(word)<=2)
    text = " ".join(ma.parse(word)[0].normal_form for word in text.split())
    # text = " ".join(word for word in text.split() if word not in stop_words)
    text = ' '.join(text.split()[:words_count])
    return text

clean_text_udf = F.udf(clean_text, returnType=T.StringType())
# if you want to run local use spark-submit or something


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command()
@click.option('--support_issues_path', default="//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues")
@click.option('--tickets_prod_path', default="//home/cloud/billing/exported-support-tables/tickets_prod")
@click.option('--components_path', default="//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/components")
@click.option('--result_path', default="//home/cloud_analytics/ml/support_tickets_classification/dataset")
def main(support_issues_path: str, tickets_prod_path: str, components_path: str, result_path: str):
 
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM, driver_memory='2G') as spark:

        issues = (
            spark.read
            .schema_hint({'components': T.ArrayType(T.StringType()),
                          'tags': T.ArrayType(T.StringType())})
            .yt(support_issues_path)
            .select('key', 'tags', F.array_contains(col('tags'), 'ml-reply').alias('verified'),
                    F.explode('components').alias('components'))
        )

        tickets_prod = (
            spark.read
            .yt(tickets_prod_path)
            .select('description', 'summary', 'st_key', 'iam_user_id', 'created_at')
        )

        components = (
            spark.read.yt(components_path)
            .select('id',
                    col('name').alias('component_name'),
                    col('shortId').alias('component_short_id'))
        )

        tickets_flat = (
            tickets_prod
            .join(issues, on=tickets_prod.st_key == issues.key)
            .join(components, on=issues.components == components.id)
        )

        tickets_with_components = (
            tickets_flat
            .groupBy('key', 'created_at')
            .agg(
                F.first('iam_user_id').alias('iam_user_id'),
                F.first('summary').alias('summary'),
                F.first('description').alias('description'),
                F.first('verified').alias('verified'),
                F.collect_set('component_name').alias('component_names')
            )
            .withColumn('sum_description', F.concat(col('summary'), F.lit(' '), col('description')))
        )

        cleaned_tickets = (
            tickets_with_components
            .filter(~F.isnull('created_at'))
            .withColumn('clean_text', clean_text_udf(col('sum_description').cast('string')))
            .withColumn('clean_summary', clean_text_udf(col('summary').cast('string')))
            .withColumn('creation_date', F.from_unixtime(col("created_at").cast(T.LongType())))
            .filter((col('creation_date') < datetime.strptime("15-11-2021", "%d-%m-%Y")) | (col('verified')))
            .filter(col('creation_date') > datetime.strptime("01-09-2019", "%d-%m-%Y"))
            .select(
                'key', 'iam_user_id', 'creation_date', 'summary', 'description', 'clean_text', 'clean_summary',
                col('component_names').alias('real_components'))
            .orderBy('creation_date', ascending=False)
            .limit(50000)
        )
        train, val = cleaned_tickets.randomSplit([0.9, 0.1], seed=42)
        train.coalesce(1).write.yt('//home/cloud_analytics/ml/support_tickets_classification/dataset_train', mode='overwrite')
        val.coalesce(1).write.yt('//home/cloud_analytics/ml/support_tickets_classification/dataset_val', mode='overwrite')


if __name__ == '__main__':
    main()
