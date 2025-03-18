from utils.common import show_groups

from tests.util import get_db_by_path


def test_show_all_fields(testdb_path):
    FIELDS = ["name", "master", "owners", "watchers", "expiration_date", "slaves", "tags", "hosts_count", "power",
              "ipower", "memory", "imemory", "disk", "idisk", "ssd", "issd"]

    util_options = {
        "show_fields": FIELDS,
        "db": testdb_path,
    }

    result = show_groups.jsmain(util_options)

    groupline = result[0]
    groupname = groupline[0]

    db_group = get_db_by_path(testdb_path, cached=False).groups.get_group(groupname)

    assert set(db_group.card.owners) == set(groupline[FIELDS.index("owners")].split(","))
    assert set(map(lambda x: x.card.name, db_group.slaves)) == (
    set() if len(groupline[FIELDS.index("slaves")]) == 0 else set(groupline[FIELDS.index("slaves")].split(",")))
    assert len(db_group.getHosts()) == int(groupline[FIELDS.index("hosts_count")])
