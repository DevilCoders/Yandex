Feature: System metrics feature
    Scenario: All system metrics are valid
        When create postgres cluster with name "cid1"
        And force processing for cluster type "postgresql_cluster"
        And we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [],
                "system": {
                    "cpu": {
                        "used": 0.14,
                        "timestamp": 1629121685
                    },
                    "mem": {
                        "timestamp": 1629121685,
                        "used": 2938172,
                        "total": 293829382
                    },
                    "disk": {
                        "timestamp": 1629121685,
                        "used": 192838293,
                        "total": 9829384144
                    }
                },
                "mode": {
                    "timestamp": 1629121685,
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When we GET "/v1/hostshealth?fqdns=myt-1.db.yandex.net"
        Then we get response with status 200
        And we get response with content
        """
        {
            "hosts": [
                {
                    "fqdn": "myt-1.db.yandex.net",
                    "services": [],
                    "cid": "**IGNORE**",
                    "status": "**IGNORE**",
                    "system": {
                        "cpu": {
                            "used": 0.14,
                            "timestamp": 1629121685
                        },
                        "mem": {
                            "timestamp": 1629121685,
                            "used": 2938172,
                            "total": 293829382
                        },
                        "disk": {
                            "timestamp": 1629121685,
                            "used": 192838293,
                            "total": 9829384144
                        }
                    }
                }
            ]
        }
        """

    Scenario: There are missing system metrics
        When create postgres cluster with name "cid1"
        And force processing for cluster type "postgresql_cluster"
        And we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [],
                "system": {
                    "cpu": {
                        "used": 0.14,
                        "timestamp": 1629121685
                    },
                    "disk": {
                        "timestamp": 1629121685,
                        "used": 192838293,
                        "total": 9829384144
                    }
                },
                "mode": {
                    "timestamp": 1629121685,
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When we GET "/v1/hostshealth?fqdns=myt-1.db.yandex.net"
        Then we get response with status 200
        And we get response with content
        """
        {
            "hosts": [
                {
                    "fqdn": "myt-1.db.yandex.net",
                    "services": [],
                    "cid": "**IGNORE**",
                    "status": "**IGNORE**",
                    "system": {
                        "cpu": {
                            "used": 0.14,
                            "timestamp": 1629121685
                        },
                        "disk": {
                            "timestamp": 1629121685,
                            "used": 192838293,
                            "total": 9829384144
                        }
                    }
                }
            ]
        }
        """

    Scenario: Some system metrics are broken
        When create postgres cluster with name "cid1"
        And force processing for cluster type "postgresql_cluster"
        And we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [],
                "system": {
                    "cpu": {
                        "used": 0.14,
                        "timestamp": 1629121685
                    },
                    "mem": {
                        "timestamp": 1629121685,
                        "used": 2938172,
                        "total": 0
                    },
                    "disk": {
                        "timestamp": 1629121685,
                        "used": 0,
                        "total": 9829384144
                    }
                },
                "mode": {
                    "timestamp": 1629121685,
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When we GET "/v1/hostshealth?fqdns=myt-1.db.yandex.net"
        Then we get response with status 200
        And we get response with content
        """
        {
            "hosts": [
                {
                    "fqdn": "myt-1.db.yandex.net",
                    "services": [],
                    "cid": "**IGNORE**",
                    "status": "**IGNORE**",
                    "system": {
                        "cpu": {
                            "used": 0.14,
                            "timestamp": 1629121685
                        }
                    }
                }
            ]
        }
        """

    Scenario: All system metrics are broken
        When create postgres cluster with name "cid1"
        And force processing for cluster type "postgresql_cluster"
        And we PUT "/v1/hostshealth"
        """
        {
            "ttl": 100,
            "hosthealth": {
                "cid": "cid1",
                "fqdn": "myt-1.db.yandex.net",
                "services": [],
                "system": {
                    "cpu": {
                        "used": 0.14,
                        "timestamp": 0
                    },
                    "mem": {
                        "timestamp": 1629121685,
                        "used": 2938172,
                        "total": 0
                    },
                    "disk": {
                        "timestamp": 1629121685,
                        "used": 0,
                        "total": 9829384144
                    }
                },
                "mode": {
                    "timestamp": 1629121685,
                    "write": true,
                    "read": true,
                    "instance_userfault_broken": false
                }
            }
        }
        """
        Then we get response with status 200
        When we GET "/v1/hostshealth?fqdns=myt-1.db.yandex.net"
        Then we get response with status 200
        And we get response with content
        """
        {
            "hosts": [
                {
                    "fqdn": "myt-1.db.yandex.net",
                    "services": [],
                    "cid": "**IGNORE**",
                    "status": "**IGNORE**"
                }
            ]
        }
        """
