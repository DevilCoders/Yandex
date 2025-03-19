UPDATE
    mdb.containers
SET project_id       = extra_properties ->> 'project_id',
    extra_properties = extra_properties - 'project_id'
WHERE extra_properties ->> 'project_id' IS NOT NULL;

UPDATE
    mdb.containers
SET managing_project_id = extra_properties ->> 'managing_project_id',
    extra_properties    = extra_properties - 'managing_project_id'
WHERE extra_properties ->> 'managing_project_id' IS NOT NULL;
