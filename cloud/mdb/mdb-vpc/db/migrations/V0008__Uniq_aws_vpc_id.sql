CREATE UNIQUE INDEX networks_aws_vpc_id on vpc.networks (NULLIF(external_resources ->> 'vpc_id', ''), region_id);
