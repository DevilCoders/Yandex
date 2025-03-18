import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Hint from 'app/components/hint/hint';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';

import i18n from 'app/lib/i18n';

import './config-editor__title.css';

class ConfigEditorTitle extends React.PureComponent {
    render() {
        const i18nHint = this.props.i18nHint ? this.props.i18nHint : 'hints';
        const i18nTitle = this.props.i18nTitle ? this.props.i18nTitle : 'titles';
        return (
            <Bem block='config-editor-title'>
                {this.props.title ?
                    <Bem
                        key='title'
                        block='config-editor-title'
                        elem='title'>
                        <span
                            title={this.props.yamlKey}>
                            {i18n(i18nTitle, this.props.title)}
                        </span>
                    </Bem> : null}
                {this.props.hint &&
                    <Bem
                        block='config-editor-title'
                        elem='hint'>
                        <Hint
                            text={i18n(i18nHint, this.props.hint)}
                            to='right'
                            scope={this.context.scope} />
                    </Bem>}
                {this.props.setDefaultValue &&
                    <Bem
                        block='config-editor-title'
                        elem='action'>
                        <Bem
                            block='config-editor-title'
                            elem='description'>
                            {`${i18n('config-editor', 'unset')}:`}
                        </Bem>
                        <Checkbox
                            theme='normal'
                            size='s'
                            tone='grey'
                            view='default'
                            disabled={this.props.readOnly}
                            checked={this.props.isDefaultValue}
                            onChange={this.props.setDefaultValue} />
                    </Bem>}
            </Bem>
        );
    }
}

ConfigEditorTitle.contextTypes = {
    scope: PropTypes.object
};

ConfigEditorTitle.propTypes = {
    title: PropTypes.string,
    yamlKey: PropTypes.string,
    hint: PropTypes.string,
    i18nHint: PropTypes.string,
    i18nTitle: PropTypes.string,
    setDefaultValue: PropTypes.func,
    isDefaultValue: PropTypes.bool,
    readOnly: PropTypes.bool
};

export default ConfigEditorTitle;
