from cloud.mdb.salt.salt.components.downtimer.downtimer import (
    vhost_from_conductor_group,
    extract_ids_from_group,
    get_downtime_from_response,
    Downtime,
)


def test_vhost_from_conductor_group():
    assert vhost_from_conductor_group("mdb_postgresql_compute_prod") == "mdb-postgresql-compute-prod"


class Test_extract_ids_from_group:
    def test_for_cluster_group(self):
        assert extract_ids_from_group("db_xxx") == "xxx"

    def test_for_non_cluster_group(self):
        assert extract_ids_from_group("mdb_postgresql_compute_prod") is None


def test_get_downtime_from_response():
    assert get_downtime_from_response(
        {
            "items": [
                {
                    "filters": [
                        {
                            "host": "mdb-sqlserver-compute-infratest",
                            "service": "",
                            "instance": "",
                            "namespace": "",
                            "tags": [],
                            "project": "",
                        }
                    ],
                    "start_time": 1632216428.0,
                    "end_time": 1632220028.0,
                    "description": "empty group downtime [MDB-10563]",
                    "user": "robot-pgaas-deploy",
                    "downtime_id": "6149a56cb4c6fbc3c48972ec",
                    "source": "",
                    "startrek_ticket": "",
                    "project": "",
                    "warnings": [],
                },
                {
                    "filters": [
                        {
                            "host": "mdb-sqlserver-compute-infratest",
                            "service": "",
                            "instance": "",
                            "namespace": "",
                            "tags": [],
                            "project": "",
                        }
                    ],
                    "start_time": 1632216245.0,
                    "end_time": 1632219845.0,
                    "description": "empty group downtime [MDB-10563]",
                    "user": "robot-pgaas-deploy",
                    "downtime_id": "6149a4b6abe5050364a63069",
                    "source": "",
                    "startrek_ticket": "",
                    "project": "",
                    "warnings": [],
                },
                {
                    "filters": [
                        {
                            "host": "mdb-sqlserver-compute-infratest",
                            "service": "",
                            "instance": "",
                            "namespace": "",
                            "tags": [],
                            "project": "",
                        }
                    ],
                    "start_time": 1632216067.0,
                    "end_time": 1632219667.0,
                    "description": "empty group downtime [MDB-10563]",
                    "user": "robot-pgaas-deploy",
                    "downtime_id": "6149a4031c658d606e8e635d",
                    "source": "",
                    "startrek_ticket": "",
                    "project": "",
                    "warnings": [],
                },
            ]
        },
        "mdb-sqlserver-compute-infratest",
        "empty group downtime [MDB-10563]",
    ) == Downtime("6149a56cb4c6fbc3c48972ec", 1632220028.0)
