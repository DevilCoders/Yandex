import React from 'react';
import PropTypes from 'prop-types';
import connect from 'react-redux/es/connect/connect';

import {setGlobalErrors} from 'app/actions/errors';
import servicesApi from 'app/api/service';

import Bem from 'app/components/bem/bem';
import ColorizedText from 'app/components/colorized-text/colorized-text';
import Button from 'lego-on-react/src/components/button/button.react';
import Select from 'lego-on-react/src/components/select/select.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import MonacoEditor from 'app/components/monaco-editor/monaco-editor';
import Link from 'lego-on-react/src/components/link/link.react';
import ServiceInfoMonitoring from './__monitoring/service-info__monitoring';

import {getService} from 'app/reducers';
import {getTrend} from 'app/reducers/service';

import Markdown from 'react-markdown';

import i18n from 'app/lib/i18n';

import './service-info.css';
import 'app/components/icon/_theme/icon_theme_edit.css';

import {STATUS_COLORS, SUPPORTT_PRIORITY} from 'app/enums/service';

import {serviceType} from 'app/types';
import {STATUS} from 'app/enums/service';
import {closeConfirmDialog, openConfirmDialog} from 'app/actions/confirm-dialog';
import {openTicketCreation} from 'app/actions/ticket-creation';
import {
    fetchTrend,
    disableService,
    enableService,
    addServiceComment,
    setServiceEnableMonitoring,
    serviceChangePriority
} from 'app/actions/service';

import {getUser} from 'app/lib/user';
import {getPermissions} from 'app/lib/permissions';

class ServiceInfo extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            oldComment: '',
            comment: '',
            edit: false
        };

        this.onDisableClick = this.onDisableClick.bind(this);
        this.onEnableClick = this.onEnableClick.bind(this);
        this.onChangeComment = this.onChangeComment.bind(this);
        this.onChangePriority = this.onChangePriority.bind(this);
        this.onChangeEnableMonitoring = this.onChangeEnableMonitoring.bind(this);
        this.onSave = this.onSave.bind(this);
        this.onCancel = this.onCancel.bind(this);
        this.onCreateTicket = this.onCreateTicket.bind(this);
        this.onEdit = this.onEdit.bind(this);
        this.getComment = this.getComment.bind(this);

        const user = getUser();
        const permissions = getPermissions();
        if (user.hasPermission(permissions.SERVICE_COMMENT)) {
            this.getComment();
        }
    }

    componentDidMount() {
        const user = getUser();
        const permissions = getPermissions();

        if (user.hasPermission(permissions.TICKET_SEE)) {
            this.props.fetchTrend(this.props.service.id);
        }
    }

    onDisableClick() {
        const {
            service,
            disableService
        } = this.props;

       disableService(service.id);
    }

    onEnableClick() {
        const {
            service,
            enableService
        } = this.props;

        enableService(service.id);
    }

    onChangePriority(value) {
        this.props.serviceChangePriority(this.props.service.id, value[0]);
    }

    onChangeComment(value) {
        this.setState({
            comment: value
        });
    }

    onChangeEnableMonitoring(monitoringName, value) {
        const {
            setServiceEnableMonitoring,
            service
        } = this.props;

        setServiceEnableMonitoring(service.id, monitoringName, value);
    }

    onSave() {
        const {
            service,
            addServiceComment
        } = this.props;
        const {
            comment
        } = this.state;

        if (comment !== service.comment) {
            addServiceComment(service.id, comment);
        }
        this.setState(state => ({
            oldComment: state.comment,
            edit: false
        }));
    }

    onCancel() {
        this.setState(state => ({
            comment: state.oldComment,
            edit: false
        }));
    }

    onEdit() {
        this.setState({
            edit: true
        });
    }

    onCreateTicket() {
        this.props.openTicketCreation(this.props.service.id);
    }

    getComment() {
        servicesApi.getServiceComment(this.props.service.id).then(result => {
            this.setState({
                oldComment: result.comment,
                comment: result.comment
            });
        }, error => {
            this.setState({
                oldComment: '',
                comment: ''
            });
            this.props.setGlobalErrors([error.message]);
        });
    }

    renderEditor() {
        const {
            comment
        } = this.state;

        return (
            <Bem
                block='service-info'
                elem='comment'>
                <Bem
                    block='service-info'
                    elem='comment-editor'>
                    <MonacoEditor
                        language='markdown'
                        options={{
                            quickSuggestions: {
                                other: false,
                                comments: false,
                                strings: true
                            },
                            automaticLayout: true,
                            scrollBeyondLastLine: false
                        }}
                        value={comment}
                        onChange={this.onChangeComment} />
                </Bem>
                <Bem
                    block='service-info'
                    elem='save'>
                    <Button
                        theme='action'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={this.onSave}>
                        {i18n('common', 'save')}
                    </Button>
                </Bem>
                <Bem
                    block='service-info'
                    elem='cancel'>
                    <Button
                        theme='action'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={this.onCancel}>
                        {i18n('common', 'cancel')}
                    </Button>
                </Bem>
            </Bem>
        );
    }

    // TODO По идее это нужно выносить в отдельные компоненты
    renderViewer() {
        const {
            comment
        } = this.state;

        return (
            <Bem
                block='service-info'
                elem='comment-viewer'>
                <Bem
                    block='service-info'
                    elem='markdown-viewer'>
                    <Markdown
                        source={comment} />
                </Bem>
                <Bem
                    key='actions'
                    block='service-info'
                    elem='edit'>
                    <Button
                        theme='raised'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'service-info',
                            elem: 'action'
                        }}
                        onClick={this.onEdit}>
                        <Icon
                            key='icon'
                            mix={[{
                                mods: {
                                    theme: 'edit'
                                }
                            }, {
                                block: 'service-info',
                                elem: 'icon'
                            }]}
                            size='s' />
                    </Button>
                </Bem>
            </Bem>
        );
    }

    render() {
        const user = getUser();
        const permissions = getPermissions();
        const {
            service,
            trend
        } = this.props;

        return (
            <Bem
                block='service-info'>
                <Bem
                    key='form'
                    block='service-info'
                    elem='form'>
                    {user.hasPermission(permissions.SERVICE_COMMENT) && (
                        <Bem
                            block='service-info'
                            elem='service-comment'>
                            {this.state.edit ?
                                this.renderEditor() :
                                this.renderViewer()}
                        </Bem>
                    )}
                    <Bem
                        block='service-info'
                        elem='info-actions'>
                        <Bem
                            block='service_info'
                            elem='info'>
                            <Bem
                                key='id'
                                block='service-info'
                                elem='form-row'>
                                <Bem
                                    key='label'
                                    block='service-info'
                                    elem='form-row-label'>
                                    ID
                                </Bem>
                                <Bem
                                    key='value'
                                    block='service-info'
                                    elem='form-row-value'>
                                    {service.id}
                                </Bem>
                            </Bem>

                            <Bem
                                key='name'
                                block='service-info'
                                elem='form-row'>
                                <Bem
                                    key='label'
                                    block='service-info'
                                    elem='form-row-label'>
                                    {i18n('common', 'name')}
                                </Bem>
                                <Bem
                                    key='value'
                                    block='service-info'
                                    elem='form-row-value'>
                                    {service.name}
                                </Bem>
                            </Bem>

                            <Bem
                                key='status'
                                block='service-info'
                                elem='form-row'>
                                <Bem
                                    key='label'
                                    block='service-info'
                                    elem='form-row-label'>
                                    {i18n('common', 'status')}
                                </Bem>
                                <Bem
                                    key='value'
                                    block='service-info'
                                    elem='form-row-value'>
                                    <ColorizedText color={STATUS_COLORS[service.status]}>
                                        {i18n('service-page', `service-status-${service.status}`)}
                                    </ColorizedText>
                                </Bem>
                            </Bem>
                            {service.support_priority && user.hasPermission(permissions.SUPPORT_PRIORITY_SWITCH) &&
                                <Bem
                                    key='support_priority'
                                    block='service-info'
                                    elem='form-row'>
                                    <Bem
                                        key='label'
                                        block='service-info'
                                        elem='form-row-label'>
                                        {i18n('service-page', 'service-priority')}
                                    </Bem>
                                    <Bem
                                        key='value'
                                        block='service-info'
                                        elem='form-row-value'>
                                        <Select
                                            theme='pseudo'
                                            view='default'
                                            tone='grey'
                                            size='m'
                                            width='max'
                                            type='radio'
                                            val={service.support_priority}
                                            progress={service.settingSupportPriority}
                                            onChange={this.onChangePriority}>
                                            {Object.keys(SUPPORTT_PRIORITY).map(priority => (
                                                <Select.Item
                                                    key={priority}
                                                    val={SUPPORTT_PRIORITY[priority]}>
                                                    {i18n('placeholders', priority)}
                                                </Select.Item>
                                            ))}
                                        </Select>
                                    </Bem>
                                </Bem>
                            }
                            {trend && trend.loaded && user.hasPermission(permissions.TICKET_SEE) &&
                                <Bem
                                    key='trend'
                                    block='service-info'
                                    elem='form-row'>
                                    <Bem
                                        key='label'
                                        block='service-info'
                                        elem='form-row-label'>
                                        {i18n('service-page', 'service-trend')}
                                    </Bem>
                                    <Bem
                                        key='value'
                                        block='service-info'
                                        elem='form-row-value'>
                                        {trend.ticket_id ?
                                            <Link
                                                theme='black'
                                                url={`https://st.yandex-team.ru/${trend.ticket_id}`}
                                                target='_blank'>
                                                {trend.ticket_id}
                                            </Link> :
                                            <Button
                                                theme='action'
                                                view='default'
                                                tone='grey'
                                                size='s'
                                                progress={trend.creating}
                                                disabled={!user.hasPermission(permissions.TICKET_CREATE)}
                                                onClick={this.onCreateTicket}>
                                                {i18n('common', 'create')}
                                            </Button>
                                        }
                                    </Bem>
                                </Bem>
                            }
                            <Bem
                                block='service-info'
                                elem='status-action'>
                                <ServiceInfoMonitoring
                                    settingEnableMonitoring={service.settingEnableMonitoring}
                                    monitoringsEnabled={service.monitorings_enabled}
                                    optionalMonitorings={service.optionalMonitorings}
                                    onChangeEnableMonitoring={this.onChangeEnableMonitoring} />
                                {service.status === STATUS.ACTIVE ?
                                    <Button
                                        view='default'
                                        tone='red'
                                        size='s'
                                        theme='normal'
                                        onClick={this.onDisableClick}
                                        mix={{
                                            block: 'service-info',
                                            elem: 'status-action-button'
                                        }}>
                                        {i18n('common', 'disable')}
                                    </Button> :
                                    <Button
                                        view='default'
                                        tone='grey'
                                        size='s'
                                        theme='action'
                                        onClick={this.onEnableClick}
                                        mix={{
                                            block: 'service-info',
                                            elem: 'status-action-button'
                                        }}>
                                        {i18n('common', 'enable')}
                                    </Button>
                                }
                            </Bem>
                        </Bem>
                    </Bem>
                </Bem>
            </Bem>
        );
    }
}

ServiceInfo.propTypes = {
    service: serviceType,
    trend: PropTypes.object,
    disableService: PropTypes.func,
    enableService: PropTypes.func,
    addServiceComment: PropTypes.func,
    openTicketCreation: PropTypes.func,
    fetchTrend: PropTypes.func,
    setGlobalErrors: PropTypes.func,
    setServiceEnableMonitoring: PropTypes.func,
    serviceChangePriority: PropTypes.func
};

export default connect(state => {
    return {
        trend: getTrend(getService(state))
    };
}, dispatch => {
    return {
        disableService: serviceId => {
            dispatch(openConfirmDialog(i18n('service-page', 'disable-confirmation-message'), result => {
                dispatch(closeConfirmDialog());

                if (result) {
                    dispatch(disableService(serviceId));
                }
            }));
        },
        serviceChangePriority: (serviceId, priority) => {
            dispatch(serviceChangePriority(serviceId, priority));
        },
        openTicketCreation: serviceId => {
            dispatch(openTicketCreation(serviceId));
        },
        enableService: serviceId => {
            dispatch(enableService(serviceId));
        },
        addServiceComment: (serviceId, comment) => {
            dispatch(addServiceComment(serviceId, comment));
        },
        fetchTrend: serviceId => {
            dispatch(fetchTrend(serviceId));
        },
        setServiceEnableMonitoring: (serviceId, monitoringName, monitoringIsEnable) => {
            dispatch(setServiceEnableMonitoring(serviceId, monitoringName, monitoringIsEnable));
        },
        setGlobalErrors: errors => {
            return dispatch(setGlobalErrors(errors));
        }
    };
})(ServiceInfo);
