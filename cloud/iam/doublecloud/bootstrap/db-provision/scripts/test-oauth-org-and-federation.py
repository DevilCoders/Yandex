ORGANIZATION_ID='FILL_ME'
FEDERATION_ID  ='FILL_ME'
PROJECT_ID     ='FILL_ME'
NAME           ='FILL_ME'
CLIENT_ID      ='FILL_ME'
CLIENT_SECRET  ='FILL_ME'
IS_GLOBAL      ='FILL_ME'

sql = """
INSERT INTO `org/organizations`
    (`id`,`created_at`,`description`,`display_name`,`name`,`settings`,`status`)
VALUES ('{organization_id}',Timestamp('2021-01-01T00:00:00.000000Z'),'{name}', 'Test {name} organization','test-{name}-organization','{{}}','ACTIVE');

INSERT INTO `hardware/default/identity/r3/clouds`
    (`id`,`created_at`,`default_zone`,`description`,`name`,`organization_id`,`settings`,`status`,`user_listing_allowed`)
VALUES ('{project_id}',1609459200,'test-zone-a','Test {name}','test-{name}-project','{organization_id}','{{"default_zone":"test-zone-a"}}','ACTIVE',True);
INSERT INTO `hardware/default/identity/r3/cloud_names_non_unique`
    (`id`,`created_at`,`default_zone`,`description`,`name`,`organization_id`,`settings`,`status`,`user_listing_allowed`)
VALUES ('{project_id}',1609459200,'test-zone-a','Test {name}','test-{name}-project','{organization_id}','{{"default_zone":"test-zone-a"}}','ACTIVE',True);
INSERT INTO `hardware/default/identity/r3/clouds_organization_index`
    (`id`,`created_at`,`default_zone`,`description`,`name`,`organization_id`,`settings`,`status`,`user_listing_allowed`)
VALUES ('{project_id}',1609459200,'test-zone-a','Test {name}','test-{name}-project','{organization_id}','{{"default_zone":"test-zone-a"}}','ACTIVE',True);
UPSERT INTO `resource_manager/cloud_registry`
    ( `id`, `created_at`, `organization_id` )
VALUES ('{project_id}',Timestamp('2021-01-01T00:00:00.000000Z'),'{organization_id}' );


INSERT INTO `hardware/default/identity/r3/folders`
    (`id`,`cloud_id`,`description`,`name`,`labels`,`settings`,`status`,`created_at`,`modified_at`)
VALUES ('{project_id}','{project_id}','Test {name}','test-{name}-project','{{}}','{{}}','ACTIVE',1609459200,1609459200);
INSERT INTO `hardware/default/identity/r3/folder_names`
    (`id`,`cloud_id`,`description`,`name`,`labels`,`settings`,`status`,`created_at`,`modified_at`)
VALUES ('{project_id}','{project_id}','Test {name}','test-{name}-project','{{}}','{{}}','ACTIVE',1609459200,1609459200);
INSERT INTO `hardware/default/identity/r3/folders_cloud_index`
    (`id`,`cloud_id`,`description`,`name`,`labels`,`settings`,`status`,`created_at`,`modified_at`)
VALUES ('{project_id}','{project_id}','Test {name}','test-{name}-project','{{}}','{{}}','ACTIVE',1609459200,1609459200);
UPSERT INTO `resource_manager/folder_registry`
    ( `id`, `cloud_id`, `created_at`)
VALUES ('{project_id}','{project_id}',Timestamp('2021-01-01T00:00:00.000000Z'));


INSERT INTO `org/federations/all`
(id, created_at, organization_id, type)
VALUES
(
'{federation_id}',
cast('2021-08-30T12:24:52.317000Z' AS Timestamp),
'{organization_id}',
'oauth'
);

INSERT INTO `org/federations/oauth`
(id, authorization_endpoint, client_authentication, client_id, client_secret, created_at, default_claims_mapping, description, name, organization_id, scopes, status, token_endpoint, userinfo_endpoint, cookie_max_age,autocreate_users,is_global)
VALUES
(
'{federation_id}',
'https://accounts.google.com/o/oauth2/v2/auth',
'client_secret_post',
'{client_id}',
'{client_secret}',
cast('2021-08-30T12:24:52.317000Z' AS Timestamp),
'{{"ext_id":"sub", "preferred_username":"email", "name":"name", "email":"email", "picture":"picture"}}',
'google-client-{name}',
'google-client-{name}',
'{organization_id}',
'["openid","profile","email"]',
'ACTIVE',
'https://oauth2.googleapis.com/token',
null,
36500,
true,
{is_global}
);
INSERT INTO `org/federations/oauth_org_index`
(id, authorization_endpoint, client_authentication, client_id, client_secret, created_at, default_claims_mapping, description, name, organization_id, scopes, status, token_endpoint, userinfo_endpoint, cookie_max_age,autocreate_users,is_global)
VALUES
(
'{federation_id}',
'https://accounts.google.com/o/oauth2/v2/auth',
'client_secret_post',
'{client_id}',
'{client_secret}',
cast('2021-08-30T12:24:52.317000Z' AS Timestamp),
'{{"ext_id":"sub", "preferred_username":"email", "name":"name", "email":"email", "picture":"picture"}}',
'google-client-{name}',
'google-client-{name}',
'{organization_id}',
'["openid","profile","email"]',
'ACTIVE',
'https://oauth2.googleapis.com/token',
null,
36500,
true,
{is_global}
);
""".format(
    organization_id=ORGANIZATION_ID,federation_id=FEDERATION_ID,project_id=PROJECT_ID,name=NAME,
    client_id=CLIENT_ID,client_secret=CLIENT_SECRET,is_global=IS_GLOBAL)

print(sql)
