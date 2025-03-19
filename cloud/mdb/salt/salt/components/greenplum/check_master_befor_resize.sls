{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

gpdb_check_master_replica_is_up:
  mdb_greenplum.check_master_replica_is_alive_call
