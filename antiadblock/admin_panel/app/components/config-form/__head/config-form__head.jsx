import React from 'react';
import PropTypes from 'prop-types';

import i18n from 'app/lib/i18n';

import Bem from 'app/components/bem/bem';
import Select from 'lego-on-react/src/components/select/select.react';
import {STATUS} from 'app/enums/config';

import './config-form__head.css';

class ConfigFormHead extends React.Component {
    renderInfo(text) {
        return (
            <Bem
                block='config-form-header'
                elem='info'>
                {text}
            </Bem>
        );
    }

    render() {
        return (
            <Bem
                key='header'
                block='config-form-header'>
                <Bem
                    block='config-form-header'
                    elem='title'>
                    {i18n('service-page', 'config-form-header')}
                </Bem>
                <Bem
                    block='config-form-header'
                    elem='action'>
                    <Bem
                        block='config-form-header'
                        elem='action-title'>
                        {i18n('config-editor', 'show-dependence-for')}:
                    </Bem>
                    <Select
                        theme='pseudo'
                        view='default'
                        tone='grey'
                        size='s'
                        type='radio'
                        val={this.props.activeParentConfigType}
                        onChange={this.props.onChangeParentConfigType} >
                        {[STATUS.ACTIVE, STATUS.TEST, STATUS.EXPERIMENT].map(key => (
                            <Select.Item
                                key={key}
                                val={key}>
                                {`${i18n('config-editor', key)}`}
                            </Select.Item>
                        ))}
                    </Select>
                    {this.props.activeParentConfigType === STATUS.EXPERIMENT && (
                        this.props.parentConfigsExpId.loaded ? (
                            this.props.parentConfigsExpId.data.total ?
                                <Bem
                                    block='config-form-header'
                                    elem='select-exp'>
                                    <Select
                                        theme='pseudo'
                                        view='default'
                                        tone='grey'
                                        size='s'
                                        type='radio'
                                        val={this.props.activeParentConfigExpId}
                                        onChange={this.props.onChangeParentConfigExpId} >
                                        {this.props.parentConfigsExpId.data.items.map(id => (
                                            <Select.Item
                                                key={id}
                                                val={id}>
                                                {id}
                                            </Select.Item>
                                        ))}
                                    </Select>
                                </Bem> : this.renderInfo(i18n('config-editor', 'not-found-exp-conf')
                            )
                        ) : this.renderInfo(`${i18n('common', 'load')}...`)
                    )}
                </Bem>
            </Bem>
        );
    }
}

ConfigFormHead.propTypes = {
    activeParentConfigType: PropTypes.string,
    activeParentConfigExpId: PropTypes.string,
    onChangeParentConfigExpId: PropTypes.func.isRequired,
    onChangeParentConfigType: PropTypes.func.isRequired,
    parentConfigsExpId: PropTypes.object.isRequired
};

export default ConfigFormHead;
