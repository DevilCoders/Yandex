DROP EVENT IF EXISTS mysql.mdb_repl_mon_event;
DROP TABLE IF EXISTS mysql.mdb_repl_mon;

CREATE TABLE mysql.mdb_repl_mon(
        id INT NOT NULL PRIMARY KEY,
        ts TIMESTAMP(3)
)
ENGINE=INNODB;

INSERT INTO mysql.mdb_repl_mon VALUES(1, CURRENT_TIMESTAMP(3));
