#include <sstream>
#include <iostream>

#include <geobase/include/lookup_wrapper.hpp>
#include <geobase/include/service_getter.hpp>
#include <geobase/include/timezone_getter.hpp>

#include "ngx_http_geobase_module.h"


using namespace NGeobase::NImpl;
using namespace NGeobase::NImpl::NRegionTraits;
using namespace std;

string
joinIdsList(const IdsList &ids) {
    ostringstream out;
    for (size_t i = 0; i < ids.size(); i++) {
        out << ids[i];
        if (i < ids.size() - 1) {
            out << ',';
        }
    }
    return out.str();
}

string
getPathInfo(TLookup *lookUp, Id regionId,bool isName) {
    static NGeobase::ERegionType types[]={
        NGeobase::ERegionType::COUNTRY,
        NGeobase::ERegionType::COUNTRY_PART,
        NGeobase::ERegionType::REGION,
        NGeobase::ERegionType::CITY,
        NGeobase::ERegionType::REMOVED
    };

    stringstream ss;

    for(size_t i = 0; types[i] != NGeobase::ERegionType::REMOVED; i++) {
        auto id = lookUp->GetParentIdWithType(regionId, (int)types[i]);
        auto region = lookUp->GetRegionById(id);

        ss << (i == 0 ? "" : "/");
        ss << (isName ? region.GetEnName() : region.GetIsoName());
    }

    return ss.str();
}

string
getIpInfo(TLookup *lookUp, const string &ip, EGeobaseField field) {
    stringstream ss;
    auto regionId = lookUp->GetRegionIdByIp(ip);
    Id parentId;

    switch(field) {
    case GEOBASE_COUNTRY:
    case GEOBASE_COUNTRY_NAME:
        parentId = lookUp->GetParentIdWithType(regionId, (int)NGeobase::ERegionType::COUNTRY);
        break;
    case GEOBASE_COUNTRY_PART:
    case GEOBASE_COUNTRY_PART_NAME:
        parentId = lookUp->GetParentIdWithType(regionId, (int)NGeobase::ERegionType::COUNTRY_PART);
        break;
    case GEOBASE_REGION:
    case GEOBASE_REGION_NAME:
        parentId = lookUp->GetParentIdWithType(regionId, (int)NGeobase::ERegionType::REGION);
        break;
    case GEOBASE_CITY:
    case GEOBASE_CITY_NAME:
        parentId = lookUp->GetParentIdWithType(regionId, (int)NGeobase::ERegionType::CITY);
        break;
    case GEOBASE_LATITUDE:
    case GEOBASE_LONGITUDE:
        parentId = regionId;
        break;
    case GEOBASE_PATH:
    case GEOBASE_PATH_NAME:
        return getPathInfo(lookUp, regionId, field == GEOBASE_PATH_NAME);
    case GEOBASE_REGION_ID:
        return to_string(regionId);
    case GEOBASE_PARENTS_IDS:
        return joinIdsList(lookUp->GetParentsIds(regionId));
    case GEOBASE_CHILDREN_IDS:
        return joinIdsList(lookUp->GetChildrenIds(regionId));
    default:
        return "";
    }

    auto region = lookUp->GetRegionById(parentId);

    switch(field) {
    case GEOBASE_COUNTRY:
    case GEOBASE_COUNTRY_PART:
    case GEOBASE_REGION:
    case GEOBASE_CITY:
        ss << region.GetIsoName();
        break;
    case GEOBASE_COUNTRY_NAME:
    case GEOBASE_COUNTRY_PART_NAME:
    case GEOBASE_REGION_NAME:
    case GEOBASE_CITY_NAME:
        ss << region.GetEnName();
        break;
    case GEOBASE_LATITUDE:
        ss << region.GetLatitude();
        break;
    case GEOBASE_LONGITUDE:
        ss << region.GetLongitude();
        break;
    default:
        break;
    }

    return ss.str();
}

string
getIpTrait(TLookup *lookUp, const string &ip, EGeobaseTrait trait) {
    if (trait == GEOBASE_TRAIT_ISP_NAME || trait == GEOBASE_TRAIT_ORG_NAME || trait == GEOBASE_TRAIT_ASN_LIST) {
        auto traits = lookUp->GetTraitsByIp(ip);

        switch(trait) {
        case GEOBASE_TRAIT_ISP_NAME:
            return traits.IspName;
        case GEOBASE_TRAIT_ORG_NAME:
            return traits.OrgName;
        case GEOBASE_TRAIT_ASN_LIST:
            return traits.AsnList;
        default:
            return "";
        }
    }

    auto traits = lookUp->GetBasicTraitsByIp(ip);
    bool result;

    switch(trait) {
    case GEOBASE_TRAIT_IS_STUB:
        result = traits.IsStub();
        break;
    case GEOBASE_TRAIT_IS_RESERVED:
        result = traits.IsReserved();
        break;
    case GEOBASE_TRAIT_IS_YANDEX_NET:
        result = traits.IsYandexNet();
        break;
    case GEOBASE_TRAIT_IS_YANDEX_STAFF:
        result = traits.IsYandexStaff();
        break;
    case GEOBASE_TRAIT_IS_YANDEX_TURBO:
        result = traits.IsYandexTurbo();
        break;
    case GEOBASE_TRAIT_IS_TOR:
        result = traits.IsTor();
        break;
    case GEOBASE_TRAIT_IS_PROXY:
        result = traits.IsProxy();
        break;
    case GEOBASE_TRAIT_IS_VPN:
        result = traits.IsVpn();
        break;
    case GEOBASE_TRAIT_IS_HOSTING:
        result = traits.IsHosting();
        break;
    case GEOBASE_TRAIT_IS_MOBILE:
        result = traits.IsMobile();
        break;
    default:
        return "";
    }

    return result ? "1" : "0";
}

extern "C"
char * ngx_http_geobase_init(ngx_conf_t *cf,
                             ngx_http_geobase_conf_t* conf,
                             ngx_str_t *path)
{
    TLookup  *lookUp = (TLookup *)conf->lookUp;

    if (lookUp != nullptr) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "geobase dublicate \"%V\"", path);
        return (char *)NGX_CONF_ERROR;
    }

    try {
        conf->lookUp = new TLookup(string((const char *)path->data, path->len),
                                   TLookup::TInitTraits().Preloading(true));

    } catch (exception &e) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "ngx_http_geobase_init(\"%V\") failed '%s'", path, e.what());
        return (char *)NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

extern "C"
void
ngx_http_geobase_cleanup(void *data)
{
 ngx_http_geobase_conf_t  *conf=(ngx_http_geobase_conf_t*)data;
 TLookup                  *lookUp = (TLookup *)conf->lookUp;

    if(lookUp != nullptr) {
        delete lookUp;
        conf->lookUp = nullptr;
    }
}


template <
    typename TGeobaseEnum,
    string (*getIpInfoFunc) (TLookup *lookUp, const string &ip, TGeobaseEnum data)>
ngx_int_t
ngx_get_geobase_info_template(ngx_http_request_t *r,
                              ngx_http_geobase_conf_t *conf,
                              ngx_str_t ip,
                              uintptr_t data,
                              ngx_http_variable_value_t *val)
{
    TLookup  *lookUp = (TLookup *)conf->lookUp;

    try {
        auto result = (*getIpInfoFunc)(lookUp, string((const char *)ip.data, ip.len), (TGeobaseEnum)data);

        val->data = (u_char *)ngx_pnalloc(r->pool, result.size());
        if (val->data == NULL) {
            return NGX_ERROR;
        }

        ngx_memcpy(val->data, result.c_str(), result.size());
        val->len = result.size();
        val->valid = 1;
        val->no_cacheable = 0;
        val->not_found = 0;

    } catch (exception &) {
        val->valid = 0;
        val->no_cacheable = 0;
        val->not_found = 1;
    }

    return NGX_OK;
}


extern "C"
ngx_int_t
ngx_get_geobase_info(ngx_http_request_t *r,
                     ngx_http_geobase_conf_t *conf,
                     ngx_str_t ip,
                     uintptr_t field,
                     ngx_http_variable_value_t *val)
{
    return ngx_get_geobase_info_template<EGeobaseField, &getIpInfo>(r, conf, ip, field, val);
}

extern "C"
ngx_int_t
ngx_get_geobase_trait(ngx_http_request_t *r,
                      ngx_http_geobase_conf_t *conf,
                      ngx_str_t ip,
                      uintptr_t trait,
                      ngx_http_variable_value_t *val)
{
    return ngx_get_geobase_info_template<EGeobaseTrait, &getIpTrait>(r, conf, ip, trait, val);
}
