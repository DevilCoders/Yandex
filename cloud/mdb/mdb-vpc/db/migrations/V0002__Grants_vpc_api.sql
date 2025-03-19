GRANT USAGE ON SCHEMA vpc TO vpc_api;
GRANT SELECT ON vpc.networks TO vpc_api;
GRANT SELECT, INSERT ON vpc.operations TO vpc_api;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA vpc TO vpc_api;
