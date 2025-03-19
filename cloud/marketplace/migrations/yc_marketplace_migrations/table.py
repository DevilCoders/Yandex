class MigrationTable:
    columns = {
        "id": "UTF8",
        "created_at": "UINT64",
        "metadata": "JSON",
    }
    primary_keys = ["id"]

    def __init__(self, name="migration"):
        self.name = name

    def create(self):
        pk = "PRIMARY KEY ({})".format(", ".join(self.primary_keys))
        columns = ", ".join(["{} {}".format(field, self.columns[field]) for field in self.columns])
        return "CREATE TABLE {} ({}, {})".format(self.name, columns, pk)
