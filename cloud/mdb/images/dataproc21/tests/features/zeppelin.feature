@zeppelin @conda @pip @spark @pyspark
Feature: Cluster with zeppelin works
  Scenario: Cluster with zeppelin and dependencies creates and works
    Given cluster name "zpln"
    # External connectivity needs for installing additional libraries
    Given NAT network
    And cluster with services: hdfs, mapreduce, yarn, spark, zeppelin
    And property pip:plotnine = 0.8.0
    And property pip:ggplot = 0.11.5
    And property pip:bkzep = 0.6.1
    And property pip:holoviews = 1.14.4
    And property pip:hvplot = 0.7.2
    And property pip:altair = 4.1.0
    When cluster created within 15 minutes
    Then service zeppelin on masternodes is running
    And tunnel to 8890 is open

  Scenario: /Python Tutorial/1. IPython Basic
    Then run zeppelin note /Python Tutorial/1. IPython Basic

  @skip
  Scenario: /Python Tutorial/2. IPython Visualization Tutorial
    # Temporary skip some notes, cz zeppelin has broken examples
    # See https://github.com/bokeh/bokeh/issues/9854
    # https://github.com/zjffdu/bkzep/issues/12
    Then run zeppelin note /Python Tutorial/2. IPython Visualization Tutorial

  @skip
  # Temporary disable, cz too slow
  Scenario: /Python Tutorial/3. Keras Binary Classification (IMDB)
    Then run zeppelin note /Python Tutorial/3. Keras Binary Classification (IMDB)

  @skip @spark @pyspark
  Scenario: /Python Tutorial/4. Matplotlib (Python, PySpark)
    # Skip paragraph, cz ipython doesn't support mpl_config
    # https://issues.apache.org/jira/browse/ZEPPELIN-3873
    # Given skip zeppelin paragraph 20160616-234947_579056637
    # And skip paragraph 20161101-195657_1336292109
    # And skip paragraph 20161101-200754_739212093
    Then run zeppelin note /Python Tutorial/4. Matplotlib (Python, PySpark)

  @spark
  Scenario: /Spark Tutorial/2. Spark Basic Features
    Then run zeppelin note /Spark Tutorial/2. Spark Basic Features

  @spark @pyspark
  Scenario: /Spark Tutorial/3. Spark SQL (PySpark)
    # This paragraph require custom file with data on all of nodes
    # https://github.com/apache/spark/blob/master/examples/src/main/resources/people.json
    # Temporary skip this paragraph
    Given skip paragraph 20180530-101930_1495479697
    Then run zeppelin note /Spark Tutorial/3. Spark SQL (PySpark)

  @spark @sparksql
  Scenario: /Spark Tutorial/3. Spark SQL (Scala)
    # This paragraph require custom file with data on all of nodes
    # https://github.com/apache/spark/blob/master/examples/src/main/resources/people.json
    # Temporary skip this paragraph
    Given skip paragraph 20180530-101930_1495479697
    Then run zeppelin note /Spark Tutorial/3. Spark SQL (Scala)

  @skip @spark @mllib
  # Skip scearnio, cz it requires broken bkzep
  Scenario: /Spark Tutorial/4. Spark MlLib
    Then run zeppelin note /Spark Tutorial/4. Spark MlLib

  # MDB-8937: Compile Spark with Rlang support
  @skip @spark @rlang
  Scenario: /Spark Tutorial/5. SparkR Basics
    Then run zeppelin note /Spark Tutorial/5. SparkR Basics

  @skip @spark @rlang
  Scenario: /Spark Tutorial/6. SparkR Shiny App
    Then run zeppelin note /Spark Tutorial/6. SparkR Shiny App

  @skip @spark @deltatable
  Scenario: /Spark Tutorial/7. Spark Delta Lake Tutorial
    Then run zeppelin note /Spark Tutorial/7. Spark Delta Lake Tutorial
