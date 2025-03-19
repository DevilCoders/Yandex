# MDB-17219: Fix catboost image test
@spark @catboost @skip
Feature: Spark + Catboost works
    Scenario: Spark cluster with catboost creates
        Given cluster name "catboost"
        And NAT network
        And cluster with services: hdfs, mapreduce, yarn, spark, livy
        And property spark:spark.jars.packages = ai.catboost:catboost-spark_3.0_2.12:0.25
        When cluster created within 10 minutes
        When tunnel to 8998 is open

    # https://github.com/catboost/catboost/tree/master/catboost/spark/catboost4j-spark#binary-classification-1
    Scenario: Catboost Binary classification works
        When start livy pyspark session
        Then livy session started within 240 seconds
        Given livy code
        """
        from pyspark.sql import Row,SparkSession
        from pyspark.ml.linalg import Vectors, VectorUDT
        from pyspark.sql.types import *

        import catboost_spark

        srcDataSchema = [
            StructField("features", VectorUDT()),
            StructField("label", StringType())
        ]

        trainData = [
            Row(Vectors.dense(0.1, 0.2, 0.11), "0"),
            Row(Vectors.dense(0.97, 0.82, 0.33), "1"),
            Row(Vectors.dense(0.13, 0.22, 0.23), "1"),
            Row(Vectors.dense(0.8, 0.62, 0.0), "0")
        ]
        """
        When execute livy statement
        Then livy statement finished

        Given livy code
        """
        trainDf = spark.createDataFrame(spark.sparkContext.parallelize(trainData), StructType(srcDataSchema))
        trainPool = catboost_spark.Pool(trainDf)
            
        evalData = [
            Row(Vectors.dense(0.22, 0.33, 0.9), "1"),
            Row(Vectors.dense(0.11, 0.1, 0.21), "0"),
            Row(Vectors.dense(0.77, 0.0, 0.0), "1")
        ]
            
        evalDf = spark.createDataFrame(spark.sparkContext.parallelize(evalData), StructType(srcDataSchema))
        evalPool = catboost_spark.Pool(evalDf)
            
        classifier = catboost_spark.CatBoostClassifier()
        """
        When execute livy statement
        Then livy statement finished 
        
        Given livy code
        """"
        # train model
        model = classifier.fit(trainPool, [evalPool])
        """
        When execute livy statement
        Then livy statement finished within 600 seconds
        
        Given livy code
        """
        # apply model
        predictions = model.transform(evalPool.data)
        predictions.show()
        """"
        When execute livy statement
        Then livy statement finished
        And livy output contains
        """
        +---------------+-----+--------------------+--------------------+----------+
        |       features|label|       rawPrediction|         probability|prediction|
        +---------------+-----+--------------------+--------------------+----------+
        |[0.22,0.33,0.9]|    1|[-0.0183088880990...|[0.49084657871682...|       1.0|
        |[0.11,0.1,0.21]|    0|[0.00206924891940...|[0.50103462298302...|       0.0|
        | [0.77,0.0,0.0]|    1|[0.01595372085253...|[0.50797618373510...|       0.0|
        +---------------+-----+--------------------+--------------------+----------+
        """

        Given livy code
        """"
        # save model
        savedModelPath = "s3a://{{ bucket }}/catboost/binary_classifictaion"
        model.write().overwrite().save(savedModelPath)
        """
        When execute livy statement
        Then livy statement finished

        When execute command
        """
        hadoop fs -ls s3a://{{ bucket }}/catboost/binary_classifictaion
        """
        Then command finishes
        And command stdout contains
        """
        s3a://{{ bucket }}/catboost/binary_classifictaion/metadata
        """
        And command stdout contains
        """
        s3a://{{ bucket }}/catboost/binary_classifictaion/model
        """