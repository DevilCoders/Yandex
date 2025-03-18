import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Tumbler from 'app/components/tumbler/tumbler';

import i18n from 'app/lib/i18n';

import './service-info__monitoring.css';

class ServiceInfoMonitoring extends React.Component {
    constructor(props) {
        super(props);

        this.onChangeEnableMonitoring = this.onChangeEnableMonitoring.bind(this);
    }

    onChangeEnableMonitoring(type, value) {
        return () => {
            this.props.onChangeEnableMonitoring(type, !value);
        };
    }

    renderActionBlock(titleKey, type, enabled, disabled) {
        return (
            <Bem
                block='service-info-monitoring'
                elem='action'>
                <Bem
                    block='service-info-monitoring'
                    elem='action-title'>
                    {(type !== 'monitorings' ? '- ' : '') + i18n('common', titleKey)}
                </Bem>
                <Tumbler
                    theme='normal'
                    view='default'
                    tone='grey'
                    size='s'
                    onVal='onVal'
                    offVal='offVal'
                    disabled={disabled}
                    checked={enabled}
                    onChange={this.onChangeEnableMonitoring(type, enabled)} />
            </Bem>
        );
    }

    render() {
        const {
            settingEnableMonitoring,
            monitoringsEnabled,
            optionalMonitorings
        } = this.props;

        return (
            <Bem
                block='service-info-monitoring'>
                <Bem
                    block='service-info-monitoring'
                    elem='main'>
                    {this.renderActionBlock(
                        'monitoring',
                        'monitorings',
                        monitoringsEnabled,
                        settingEnableMonitoring
                    )}
                    {monitoringsEnabled &&
                        <Bem
                            block='service-info-monitoring'
                            elem='optional'>
                            {this.renderActionBlock(
                                'monitoring-mobile',
                                'mobile_monitorings',
                                optionalMonitorings.mobile_monitorings_enabled,
                                settingEnableMonitoring
                            )}
                        </Bem>
                    }
                </Bem>
            </Bem>
        );
    }
}

ServiceInfoMonitoring.propTypes = {
    settingEnableMonitoring: PropTypes.bool,
    monitoringsEnabled: PropTypes.bool,
    optionalMonitorings: PropTypes.object,
    onChangeEnableMonitoring: PropTypes.func
};

export default ServiceInfoMonitoring;
