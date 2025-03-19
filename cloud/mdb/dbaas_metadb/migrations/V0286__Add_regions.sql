CREATE TABLE dbaas.regions (
    region_id      serial,
    name           text NOT NULL,
    cloud_provider text NOT NULL,
    CONSTRAINT pk_region PRIMARY KEY (region_id)
);

CREATE UNIQUE INDEX uk_region_names ON dbaas.regions (name);

ALTER TABLE dbaas.geo ALTER COLUMN name SET NOT NULL;
ALTER TABLE dbaas.geo ADD COLUMN region_id integer;

ALTER TABLE dbaas.geo ADD CONSTRAINT fk_geo_region
  FOREIGN KEY (region_id)
  REFERENCES dbaas.regions (region_id);

