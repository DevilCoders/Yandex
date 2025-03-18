from library.python.spyt.executable import spyt_main

from spyt import spark_session

from pyspark.sql.functions import udf
from pyspark.sql.types import LongType

from .udfs import squared


@spyt_main
def main():
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
