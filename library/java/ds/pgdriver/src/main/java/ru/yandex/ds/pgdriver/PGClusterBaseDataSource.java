package ru.yandex.ds.pgdriver;

import org.postgresql.ds.common.BaseDataSource;

abstract class PGClusterBaseDataSource extends BaseDataSource {

    @Override
    public String getUrl() {
        return Urls.asClusterUrl(super.getUrl());
    }

    @Override
    public void setUrl(String url) {
        super.setUrl(Urls.asPostgresUrl(url));
    }
}
