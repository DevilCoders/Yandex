my_conn = db.getMongo();

rs.slaveOk()
master_conn = new Mongo(rs.isMaster().primary);

db_names = [db_name];

if(db_name == '__all__') {
	databases =  db.adminCommand({listDatabases:1}).databases.filter(function(d) {
		return  ["test", "local", "config"].indexOf(d.name) == -1;
	});

	db_names = databases.map(function(d) {return d.name});
} 

fail = false;
db_names.forEach(function(d) {
	r_db = master_conn.getDB(d);
	my_db = my_conn.getDB(d);

	if(my_db.stats().indexes != r_db.stats().indexes) {
		fail = true;
	}

});
if(fail) {
	print("fail");
} else {
	print("ok");
}
