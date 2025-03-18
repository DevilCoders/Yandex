
def test_signing_policy(fake_dss, read_fixture):

    dss = fake_dss(read_fixture('signing_policy.json'))
    policy = dss.policies.get_signing_policy()

    assert 'ActionPolicy' in policy.asdict()


def test_usesrs_policy(fake_dss, read_fixture):

    dss = fake_dss(read_fixture('users_policy.json'))
    policy = dss.policies.get_users_policy()

    assert 'AllowUserRegistration' in policy.asdict()
