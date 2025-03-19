{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% set admin_role = 'mdb_admin' %}
{% set schema = 'mdb_toolkit' %}

create or replace function {{ schema }}.gp_cancel_backend(v_pid integer) RETURNS boolean
as
$$
declare
  v_usename name;
  v_query text;
begin
  select usename, lower(query)
  into v_usename, v_query
  from pg_catalog.pg_stat_activity
  where pid = $1;

  if v_usename = '{{ gpdbvars.gpadmin }}' and v_query like '%vacuum%' then
    return pg_cancel_backend(v_pid);
  elsif v_usename = '{{ gpdbvars.gpadmin }}' and v_query like '%analyze%' then
    return pg_cancel_backend(v_pid);
  elsif v_usename <> '{{ gpdbvars.gpadmin }}' then
    return pg_cancel_backend(v_pid);
  else
    return false;
  end if;
end;
$$
language plpgsql
security definer
set search_path = pg_catalog,pg_temp;

grant execute on function {{ schema }}.gp_cancel_backend(v_pid integer) to {{ admin_role }};

create or replace function {{ schema }}.gp_terminate_backend(v_pid integer) RETURNS boolean
as
$$
declare
  v_usename name;
  v_query text;
begin
  select usename, lower(query)
  into v_usename, v_query
  from pg_catalog.pg_stat_activity
  where pid = $1;

  if v_usename = '{{ gpdbvars.gpadmin }}' and v_query like '%vacuum%' then
    return pg_terminate_backend(v_pid);
  elsif v_usename = '{{ gpdbvars.gpadmin }}' and v_query like '%analyze%' then
    return pg_terminate_backend(v_pid);
  elsif v_usename <> '{{ gpdbvars.gpadmin }}' then
    return pg_terminate_backend(v_pid);
  else
    return false;
  end if;
end;
$$
language plpgsql
security definer
set search_path = pg_catalog,pg_temp;

grant execute on function {{ schema }}.gp_terminate_backend(v_pid integer) to {{ admin_role }};


