import * as serviceSbsActions from 'app/actions/service/sbs';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const defaultConfigId = -1;
    const initialProfile = {
        data: {
            general_settings: {},
            url_settings: []
        },
        loaded: false
    };
    const initialState = {
        screenshotsChecks: {
            loaded: false,
            runIdData: {},
            logsVisible: JSON.parse(localStorage.getItem('sbs-logs-visible') || '{}')
        },
        allSbsProfiles: {
            byId: {
                [defaultConfigId]: {
                    ...initialProfile
                },
                0: {
                    ...initialProfile,
                    loaded: true
                }
            },
            validation: [],
            profile: {...initialProfile},
            openedProfileId: defaultConfigId,
            loading: false,
            readOnly: false,
            isOpen: false,
            exist: false,
            saving: false,
            profileTags: {
                data: [],
                loading: false,
                loaded: false
            },
            deleting: false
        },
        resultChecks: {
            loaded: false,
            data: {
                items: [],
                total: 0
            },
            schema: {}
        },
        logs: {
            data: {
                items: [],
                total: 0
            },
            schema: {},
            loaded: false
        },
        loadingRunChecks: false
    };

    state = state || initialState;

    switch (type) {
        case serviceSbsActions.START_SERVICE_SBS_PROFILE_TAG_DELETING: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        readOnly: true,
                        deleting: true
                    }
                }
            });
        }
        case serviceSbsActions.END_SERVICE_SBS_PROFILE_TAG_DELETING: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        readOnly: false,
                        deleting: false
                    }
                }
            });
        }
        case serviceSbsActions.OPEN_SERVICE_SBS_PROFILE: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        isOpen: true,
                        readOnly: action.readOnly,
                        openedProfileId: action.profileId,
                        validation: []
                    }
                }
            });
        }
        case serviceSbsActions.CLOSE_SERVICE_SBS_PROFILE: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        isOpen: false,
                        readOnly: false,
                        openedProfileId: defaultConfigId,
                        validation: initialState.allSbsProfiles.validation
                    }
                }
            });
        }
        case serviceSbsActions.START_SERVICE_SBS_PROFILE_LOADING: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        validation: [],
                        loading: true
                    },
                    profile: {
                        $set: {
                            ...initialProfile
                        }
                    }
                }
            });
        }
        case serviceSbsActions.END_SERVICE_SBS_PROFILE_LOADING: {
            return update(state, {
                allSbsProfiles: {
                    byId: {
                        $merge: {
                            [action.id]: {
                                ...initialProfile,
                                data: action.data,
                                loaded: true
                            }
                        }
                    },
                    profile: {
                        $set: {
                            ...initialProfile,
                            loaded: true,
                            data: action.data
                        }
                    },
                    $merge: {
                        exist: Boolean(action.id),
                        loading: false
                    }
                }
            });
        }
        case serviceSbsActions.START_SERVICE_SBS_PROFILE_SAVING: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        saving: true
                    }
                }
            });
        }
        case serviceSbsActions.END_SERVICE_SBS_PROFILE_SAVING: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        isOpen: false,
                        saving: false,
                        profile: {
                            ...initialProfile
                        }
                    }
                }
            });
        }
        case serviceSbsActions.SET_ERROR_VALIDATION_SERVICE_SBS_PROFILE: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        saving: false,
                        validation: action.validation
                    }
                }
            });
        }
        case serviceSbsActions.START_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING: {
            return update(state, {
                screenshotsChecks: {
                    $merge: {
                        loaded: false,
                        runIdData: {
                            ...state.screenshotsChecks.runIdData,
                            [action.runId]: {
                                loaded: false
                            }
                        }
                    }
                }
            });
        }
        case serviceSbsActions.START_SERVICE_SBS_RESULT_CHECKS: {
            return state;
        }
        case serviceSbsActions.END_SERVICE_SBS_RESULT_CHECKS: {
            return update(state, {
                resultChecks: {
                    $merge: {
                        ...action.data,
                        loaded: true
                    }
                }
            });
        }
        case serviceSbsActions.END_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING: {
            return update(state, {
                screenshotsChecks: {
                    $merge: {
                        loaded: true,
                        runIdData: {
                            ...state.screenshotsChecks.runIdData,
                            [action.runId]: {
                                ...action.data,
                                loaded: true
                            }
                        }
                    }
                }
            });
        }
        case serviceSbsActions.START_SERVICE_SBS_RUN_CHECKS: {
            return update(state, {
                $merge: {
                    loadingRunChecks: true
                }
            });
        }
        case serviceSbsActions.END_SERVICE_SBS_RUN_CHECKS: {
            return update(state, {
                $merge: {
                    loadingRunChecks: false
                }
            });
        }
        case serviceSbsActions.CHANGE_LOG_VISIBILITY: {
            return update(state, {
                screenshotsChecks: {
                    logsVisible: {
                        $merge: {
                            ...state.screenshotsChecks.logsVisible,
                            [action.key]: action.value
                        }
                    }
                }
            });
        }
        case serviceSbsActions.START_SBS_LOGS_LOADING: {
            return update(state, {
                logs: {
                    $merge: {
                        ...initialState.logs
                    }
                }
            });
        }
        case serviceSbsActions.END_SBS_LOGS_LOADING: {
            return update(state, {
                logs: {
                    $set: {
                        ...action.data,
                        loaded: true
                    }
                }
            });
        }
        case serviceSbsActions.START_SERVICE_SBS_PROFILE_TAGS_LOADING: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        profileTags: {
                            loading: true,
                            loaded: false,
                            data: []
                        }
                    }
                }
            });
        }
        case serviceSbsActions.END_SERVICE_SBS_PROFILE_TAGS_LOADING: {
            return update(state, {
                allSbsProfiles: {
                    $merge: {
                        profileTags: {
                            loading: false,
                            loaded: true,
                            data: action.data
                        }
                    }
                }
            });
        }
        default: {
            return state;
        }
    }
}
