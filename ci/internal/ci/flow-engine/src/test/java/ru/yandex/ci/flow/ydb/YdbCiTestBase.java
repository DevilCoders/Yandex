package ru.yandex.ci.flow.ydb;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.YdbCiTestConfig;

@ContextConfiguration(classes = {
        YdbCiTestConfig.class
})
public class YdbCiTestBase extends CommonYdbTestBase {

    @SuppressWarnings("HidingField") // Different type, CiMainDb -> CiDb
    @Autowired
    protected CiDb db;
}

