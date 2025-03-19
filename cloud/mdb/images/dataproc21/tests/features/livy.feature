@livy @spark @yarn @pyspark
Feature: Livy test
    Scenario: Create minimal cluster with livy and spark
        Given cluster name "livy"
        And cluster with services: hdfs, yarn, spark, livy
        When cluster created within 10 minutes
        Then service livy on masternodes is running
        When tunnel to 8998 is open

    Scenario: Livy spark session works
        When start livy spark session my-session
        Then livy session started
        Given livy code
        """
        1 + 1
        """
        When execute livy statement
        Then livy statement finished within 15 seconds
        And  livy output contains
        """
        res1: Int = 2
        """
        Given livy code
        """
        val NUM_SAMPLES = 1000000;
        val count = sc.parallelize(1 to NUM_SAMPLES).map { i =>
            val x = Math.random();
            val y = Math.random();
            if (x*x + y*y < 1) 1 else 0
        }.reduce(_ + _);
        println("Pi is roughly " + 4.0 * count / NUM_SAMPLES)
        """
        When execute livy statement
        Then livy statement finished within 60 seconds
        And  livy output contains
        """
        Pi is roughly 3.1
        """
        Then stop livy session

    Scenario: Livy pyspark session works
        When start livy pyspark session pyspark-pi-calculation
        Then livy session started
        Given livy code
        """
        import random
        NUM_SAMPLES = 1000000

        def sample(p):
            x, y = random.random(), random.random()
            return 1 if x*x + y*y < 1 else 0

        count = sc.parallelize(range(0, NUM_SAMPLES)).map(sample).reduce(lambda a, b: a + b)
        pi = 4.0 * count / NUM_SAMPLES
        print(f"Pi is roughly {pi}")
        """
        When execute livy statement
        Then livy statement finished within 60 seconds
        And  livy output contains
        """
        Pi is roughly 3.1
        """
        Then stop livy session
