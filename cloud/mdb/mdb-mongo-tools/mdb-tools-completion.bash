#!/usr/bin/env bash


_mdb_mongo_get_completions()
{
  COMPREPLY+=("is_alive")
  COMPREPLY+=("is_ha")
  COMPREPLY+=("is_primary")
  COMPREPLY+=("primary_exists")
  COMPREPLY+=("shutdown_allowed")
}

complete -F _mdb_mongo_get_completions mdb-mongo-get
