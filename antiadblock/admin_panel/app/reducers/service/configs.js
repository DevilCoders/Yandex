import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';
import {STATUS} from 'app/enums/config';

import unionWith from 'lodash/unionWith';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        activeLabelId: null,
        byId: {},
        byLabelId: {},
        expIds: {
            loaded: false,
            data: {}
        }
    };

    const initialConfig = {
        items: [],
        loaded: false,
        total: 0
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING: {
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.START_SERVICE_CONFIG_LOADING: {
            return action.configId && state.byId[action.configId] ? update(state, {
                byId: {
                    [action.configId]: {
                        $merge: {
                            outdated: true
                        }
                    }
                }
            }) : state;
        }
        case serviceActions.END_SERVICE_CONFIG_LOADING: {
            return update(state, {
                byId: {
                    $merge: {
                        [action.configId]: action.config
                    }
                }
            });
        }
        case serviceActions.START_SERVICE_CONFIGS_LOADING: {
            const config = state.byLabelId[action.labelId] || initialConfig;

            return update(state, {
                $merge: {
                    activeLabelId: action.labelId
                },
                byLabelId: {
                    $merge: {
                        [action.labelId]: {
                            loaded: action.offset > 0,
                            items: action.offset > 0 ? config.items : initialConfig.items,
                            offset: action.offset > 0 ? config.offset : initialConfig.offset,
                            total: action.offset > 0 ? config.total : initialConfig.total
                        }
                    }
                }
            });
        }
        case serviceActions.END_SERVICE_CONFIGS_LOADING: {
            const config = state.byLabelId[action.labelId] || initialConfig;
            const union = unionWith(config.items, action.configs.items.map(config => config.id), (v1, v2) => v1.id !== v2.id);

            return update(state, {
                byId: {
                    $merge: action.configs.items.reduce((map, config) => {
                        map[config.id] = config;

                        return map;
                    }, {})
                },
                byLabelId: {
                    $merge: {
                        [action.labelId]: {
                            items: union,
                            offset: union.length,
                            total: action.configs.total,
                            loaded: true
                        }
                    }
                }
            });
        }

        case serviceActions.END_CONFIG_REMOVE_EXPERIMENT: {
            return update(state, {
                byId: {
                    [action.configId]: {
                        $merge: {
                            exp_id: undefined
                        }
                    }
                }
            });
        }

        case serviceActions.END_SERVICE_EXPERIMENT_APPLYING: {
            if (action.success) {
                return update(state, {
                    byId: {
                        [action.configId]: {
                            $merge: {
                                exp_id: action.options.comment
                            }
                        }
                    }
                });
            }

            return state;
        }

        case serviceActions.END_SERVICE_CONFIG_APPLYING: {
            let oldConfigRewriter = null,
                approvedStatus = [];

            if (action.options.oldConfigId) {
                oldConfigRewriter = {
                    [action.options.oldConfigId]: {
                        $merge: {
                            statuses: state.byId[action.options.oldConfigId].statuses.filter(status => status.status !== action.options.target)
                        }
                    }
                };
            }

            if (action.options.target === STATUS.ACTIVE) {
                approvedStatus = [
                    {
                        status: STATUS.APPROVED,
                        comment: ''
                    }
                ];
            }
            return update(state, {
                byId: {
                    [action.configId]: {
                        $merge: {
                            statuses: [
                                ...state.byId[action.configId].statuses,
                                {
                                    status: action.options.target,
                                    comment: ''
                                },
                                ...approvedStatus
                            ]
                        }
                    },
                    ...oldConfigRewriter
                }
            });
        }
        case serviceActions.START_SERVICE_CONFIG_ARCHIVED_SETTING: {
            return update(state, {
                byId: {
                    [action.configId]: {
                        $merge: {
                            processing: true
                        }
                    }
                }
            });
        }
        case serviceActions.END_SERVICE_CONFIG_ARCHIVED_SETTING: {
            return update(state, {
                byId: {
                    [action.configId]: {
                        $merge: {
                            processing: false,
                            archived: action.archived
                        }
                    }
                }
            });
        }
        case serviceActions.END_SERVICE_CONFIG_MODERATING: {
            return update(state, {
                byId: {
                    [action.configId]: {
                        $merge: {
                            statuses: [
                            ...action.config.statuses
                            ]
                        }
                    }
                }
            });
        }
        case serviceActions.START_PARENT_EXP_CONFIGS_LOADING: {
            return update(state, {
                expIds: {
                    $set: {
                        loaded: false,
                        data: {}
                    }
                }
            });
        }
        case serviceActions.END_PARENT_EXP_CONFIGS_LOADING: {
            return update(state, {
                expIds: {
                    $set: {
                        loaded: true,
                        data: action.data
                    }
                }
            });
        }
        default: {
            return state;
        }
    }
}
