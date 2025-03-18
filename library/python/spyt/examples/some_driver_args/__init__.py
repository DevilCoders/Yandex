import click

from library.python.spyt.executable import spyt_main

from .udfs import squared
from spyt import spark_session

from pyspark.sql.functions import udf
from pyspark.sql.types import LongType


def driver(test_arg):
    print("test arg: {}".format(test_arg))

    squared_udf = udf(squared, LongType())

    with spark_session() as spark:
        df = spark.createDataFrame(
            [
                (1, 'foo'),  # create your data here, be consistent in the types.
                (2, 'bar'),
            ],
            ['id', 'txt']  # add your columns label here
        )

        df.select("id", squared_udf("id").alias("id_squared")).show()
        print("Executable worked!")


@spyt_main
@click.command()
@click.option("--test_arg", required=True)
def main(test_arg):
    driver(test_arg)
