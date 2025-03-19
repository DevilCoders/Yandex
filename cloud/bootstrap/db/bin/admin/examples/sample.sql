-- Sample databse for tests

-- stands
INSERT INTO public.stands (name) VALUES ('hw6-lab');
INSERT INTO public.stands (name) VALUES ('hw4-lab');

-- hw4 lab hosts
INSERT INTO public.instances (fqdn, type, stand_id) VALUES ('man1-c624-033.cloud.yandex.net', 'host', 1);
INSERT INTO public.hw_props  VALUES (1, '{"ipv4": null, "ipv6": {"mask": null, "addr": "2a02:6b8:c01:a01:0:fe01:8371:557a"}}');

-- hw4 lab instances
INSERT INTO public.instances (fqdn, type, stand_id) VALUES ('cgw2.svc.hw4.cloud-lab.yandex.net', 'svm', 1);
INSERT INTO public.svm_props VALUES (2, '{"ipv6": {"mask": null, "addr": "2a02:6b8:bf00:1100:8839:9ff:fedb:6d61"}, "ipv4": null, "placement": "sas09-s1-41.cloud.yandex.net"}');

-- salt roles
INSERT INTO public.salt_roles (name) VALUES ('common');
INSERT INTO public.salt_roles (name) VALUES ('cloudgate');
INSERT INTO public.salt_roles (name) VALUES ('compute');

-- instance salt roles
INSERT INTO public.instance_salt_roles (instance_id, salt_role_id) VALUES (1, 1);
INSERT INTO public.instance_salt_roles (instance_id, salt_role_id) VALUES (1, 3);
INSERT INTO public.instance_salt_roles (instance_id, salt_role_id) VALUES (2, 1);
INSERT INTO public.instance_salt_roles (instance_id, salt_role_id) VALUES (2, 2);

-- instance salt role package
INSERT INTO public.instance_salt_role_packages (instance_salt_role_id, package_name, target_version) VALUES (1, 'yc-salt-formula', '0.1.2');
INSERT INTO public.instance_salt_role_packages (instance_salt_role_id, package_name, target_version) VALUES (2, 'yc-salt-formula', '0.1.3');
INSERT INTO public.instance_salt_role_packages (instance_salt_role_id, package_name, target_version) VALUES (3, 'yc-salt-formula', '1.1.1');
INSERT INTO public.instance_salt_role_packages (instance_salt_role_id, package_name, target_version) VALUES (4, 'yc-salt-formula', '2.2.2');
INSERT INTO public.instance_salt_role_packages (instance_salt_role_id, package_name, target_version) VALUES (4, 'yc-ci', '4.5.6');
