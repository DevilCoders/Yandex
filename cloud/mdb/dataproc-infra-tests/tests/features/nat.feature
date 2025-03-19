Feature: Run jobs on Dataproc cluster

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Hadoop cluster


  Scenario: Unable to create cluster if NAT is disabled
    When we try to create cluster "cluster_without_nat"
    """
    shortcuts:
      subnet_id: {{ conf['test_dataproc']['subnetWithoutNatId'] }}
    zoneId: {{ conf['test_dataproc']['subnetWithoutNatZone'] }}
    """
    Then response should have status 422 and body contains
    """
    {
      "code": 3,
      "message": "NAT should be enabled on the subnet of the main subcluster."
    }
    """
