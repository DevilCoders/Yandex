#pragma once

#include <util/generic/string.h>
#include <util/string/printf.h>

inline TString ClusterHomePath(const TString& home, int cluster) {
    return Sprintf("%s/walrus/%03d", home.data(), cluster);
}

inline TString TagHeadersDatPath(const TString& home, int cluster) {
    return ClusterHomePath(home, cluster) + "/tagheaders.dat";
}

inline TString DocTitlesDatPath(const TString& home, int cluster) {
    return ClusterHomePath(home, cluster) + "/doctitlecrc.dat";
}

inline TString ArcHeadersDatPath(const TString& home, int cluster) {
    return ClusterHomePath(home, cluster) + "/archeaders.dat";
}
