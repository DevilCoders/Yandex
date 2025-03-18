import PropTypes from 'prop-types';

export const serviceType = PropTypes.shape({
    id: PropTypes.string.isRequired,
    name: PropTypes.string.isRequired,
    status: PropTypes.string.isRequired
});

export const statusType = PropTypes.shape({
    status: PropTypes.string,
    comment: PropTypes.string
});

export const configType = PropTypes.shape({
    id: PropTypes.number.isRequired,
    data: PropTypes.object.isRequired,
    created: PropTypes.string.isRequired,
    comment: PropTypes.string.isRequired,
    statuses: PropTypes.arrayOf(statusType).isRequired,
    archived: PropTypes.bool.isRequired,
    processing: PropTypes.bool
});

export const setupType = PropTypes.shape({
    token: PropTypes.string,
    js_inline: PropTypes.string,
    nginx_config: PropTypes.string
});

export const auditItemType = PropTypes.shape({
    date: PropTypes.string,
    action: PropTypes.string,
    service_id: PropTypes.string,
    params: PropTypes.object,
    user_id: PropTypes.number
});

export const moderationType = PropTypes.shape({
    id: PropTypes.number,
    progress: PropTypes.bool,
    approve: PropTypes.bool,
    comment: PropTypes.string
});

export const heatmapItemType = PropTypes.shape({
    host: PropTypes.string,
    dc: PropTypes.string,
    resource_type: PropTypes.string,
    cpu_load: PropTypes.number,
    rps: PropTypes.number
});

export const sbsRunIdCasesType = PropTypes.arrayOf(PropTypes.shape({
    adblocker: PropTypes.string,
    adblocker_url: PropTypes.string,
    adblocker_ver: PropTypes.string,
    browser: PropTypes.string,
    browser_ver: PropTypes.string,
    end: PropTypes.string,
    img_url: PropTypes.string,
    start: PropTypes.string,
    url: PropTypes.string
}));

export const sbsRunIdDataType = PropTypes.shape({
    loaded: PropTypes.bool,
    cases: sbsRunIdCasesType,
    config_id: PropTypes.number,
    end: PropTypes.string,
    id: PropTypes.string,
    profile_id: PropTypes.number,
    sandbox_id: PropTypes.number,
    start: PropTypes.string,
    status: PropTypes.string
});

export const serviceSbsScreenshotsChecksType = PropTypes.shape({
    loaded: PropTypes.bool,
    runIdData: sbsRunIdDataType
});

export const serviceSbsProfileTagsType = PropTypes.shape({
    loading: PropTypes.bool,
    loaded: PropTypes.bool,
    data: PropTypes.arrayOf(PropTypes.string)
});

export const serviceSbsProfileType = PropTypes.shape({
    data: PropTypes.shape({
        general_settings: PropTypes.shape({
            cookies: PropTypes.string
        }).isRequired,
        url_settings: PropTypes.array
    }),
    loaded: PropTypes.bool.isRequired
});

export const serviceSbsProfilesType = PropTypes.shape({
    byId: PropTypes.shape(serviceSbsProfileType),
    profile: PropTypes.shape(serviceSbsProfileType).isRequired,
    schema: PropTypes.shape({
        path: PropTypes.shape({
            urls: PropTypes.string
        })
    }).isRequired,
    validation: PropTypes.shape({
        urls: PropTypes.arrayOf(PropTypes.shape({
            path: PropTypes.arrayOf(PropTypes.string),
            message: PropTypes.string
        }))
    }),
    openedProfileId: PropTypes.number,
    readOnly: PropTypes.bool.isRequired,
    saving: PropTypes.bool.isRequired,
    isOpen: PropTypes.bool.isRequired,
    loading: PropTypes.bool.isRequired,
    exist: PropTypes.bool.isRequired,
    deleting: PropTypes.bool.isRequired
});

export const serviceRulesSchemaType = PropTypes.shape({
    action: PropTypes.string,
    added: PropTypes.string,
    list_url: PropTypes.string,
    options: PropTypes.string,
    raw_rule: PropTypes.string,
    type: PropTypes.string,
    value: PropTypes.string
});

export const serviceRulesDataItemsType = PropTypes.arrayOf(PropTypes.shape({
    action: PropTypes.string,
    added: PropTypes.string,
    list_url: PropTypes.string,
    options: PropTypes.string,
    raw_rule: PropTypes.string,
    type: PropTypes.string,
    value: PropTypes.string
}));

export const serviceRulesType = PropTypes.shape({
    data: PropTypes.shape({
        items: serviceRulesDataItemsType,
        total: PropTypes.number
    }),
    schema: serviceRulesSchemaType,
    loaded: PropTypes.bool
});

export const serviceSbsResultChecksType = PropTypes.shape({
    loaded: PropTypes.bool.isRequired,
    data: PropTypes.shape({
        items: PropTypes.arrayOf(PropTypes.shape({
            config_id: PropTypes.number,
            date: PropTypes.string,
            id: PropTypes.number,
            owner: PropTypes.string,
            sandbox_id: PropTypes.number,
            status: PropTypes.string
        })).isRequired,
        total: PropTypes.number.isRequired
    }),
    schema: PropTypes.shape({
        config_id: PropTypes.string,
        date: PropTypes.string,
        id: PropTypes.string,
        owner: PropTypes.string,
        sandbox_id: PropTypes.string,
        status: PropTypes.string
    })
});

export const sbsMetaLogs = PropTypes.shape({
    loaded: PropTypes.bool.isRequired,
    data: PropTypes.shape({
        items: PropTypes.arrayOf(PropTypes.object),
        total: PropTypes.number
    }),
    schema: PropTypes.object
});

export const sbsScreenshotsLogs = PropTypes.shape({
    nginx: PropTypes.object,
    logs: PropTypes.object,
    balancer: PropTypes.object,
    cookies: PropTypes.object,
    cryprox: PropTypes.object
});

export const dateLibDayJsType = PropTypes.shape({
    add: PropTypes.func,
    subtract: PropTypes.func,
    format: PropTypes.func,
    daysInMonth: PropTypes.func,
    date: PropTypes.func,
    month: PropTypes.func,
    year: PropTypes.func
});
