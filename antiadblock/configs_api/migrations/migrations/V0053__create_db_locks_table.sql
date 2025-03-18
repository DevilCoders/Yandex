--table for using 'select for update' on its rows for acquiring locks
create table db_locks(id varchar(255), constraint db_locks_pkey primary key(id));