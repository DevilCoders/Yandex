my_conn = db.getMongo();
rs.slaveOk()
db_names = [db_name];

if(db_name == '__all__') {
    databases =  db.adminCommand({listDatabases:1}).databases.filter(function(d) {
        return  ["test", "local", "config"].indexOf(d.name) == -1;
    });

    db_names = databases.map(function(d) {return d.name});
}

my_version = db.version()
if (my_version[0] == 3) {
    my_engine = db.serverStatus().storageEngine.name;
} else {
    my_engine = 'mmapv1';
}


if (my_engine == 'mmapv1') {
    my_storage_size = 0;
    my_file_size = 0;
    db_names.forEach(function(d) {
        my_db = my_conn.getDB(d);
        stats = my_db.stats();
        my_storage_size += stats.storageSize;
        my_file_size += stats.fileSize;
    })
    my_fragmentation = (my_file_size/my_storage_size).toPrecision(5);
} else {
    // wiredtiger does not fragment datafiles in the same way as mmapv1
    // it does online compaction and space reuse, it might be required to fix it later
    my_fragmentation = 1;
}

print(my_fragmentation);



