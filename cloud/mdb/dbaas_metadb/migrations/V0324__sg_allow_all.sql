ALTER TABLE dbaas.sgroups
ADD COLUMN sg_allow_all boolean default false not null;

ALTER TABLE dbaas.sgroups_revs
ADD COLUMN sg_allow_all boolean default false not null;
