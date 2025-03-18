import React from 'react';

import Textarea from 'lego-on-react/src/components/textarea/textarea.react';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';
import Button from 'lego-on-react/src/components/button/button.react';
import InputWithButton from 'app/components/input-with-button/input-with-button';
import Bem from 'app/components/bem/bem';

import serviceApi from 'app/api/service';
import i18n from 'app/lib/i18n';

import './service-utils.css';

import {serviceType} from 'app/types';

export default class ServiceUtils extends React.Component {
    constructor() {
        super();

        this.state = {
            encrypted: '',
            decrypted: '',
            error: '',
            progress: false
        };

        this.setEncryptedAnchor = this.setEncryptedAnchor.bind(this);
        this.onEncryptedChange = this.onEncryptedChange.bind(this);
        this.handleKeyPress = this.handleKeyPress.bind(this);
        this.onDecrypt = this.onDecrypt.bind(this);
        this.setWrapperRef = this.setWrapperRef.bind(this);
    }

    render() {
        return (
            <Bem
                block='service-utils'>
                <Bem
                    block='service-utils'
                    elem='wrapper'
                    tagRef={this.setWrapperRef} >
                    <Bem
                        key='header'
                        block='service-page'
                        elem='header'>
                        {i18n('service-page', 'service-utils-decrypt-header')}
                    </Bem>
                    <Bem
                        key='encrypted-label'
                        block='service-utils'
                        elem='label'
                        tagRef={this.setEncryptedAnchor}>
                        {i18n('service-page', 'service-utils-encrypted-link')}
                    </Bem>
                    <Tooltip
                        anchor={this._encryptedAnchor}
                        view='classic'
                        tone='default'
                        theme='error'
                        to={['right']}
                        mainOffset={12}
                        scope={this._wrapper}
                        visible={Boolean(this.state.error)}>
                        {this.state.error}
                    </Tooltip>
                    <Bem
                        key='encrypted-value'
                        block='service-utils'
                        elem='value'>
                        <Textarea
                            theme='normal'
                            tone='grey'
                            view='default'
                            size='s'
                            text={this.state.encrypted}
                            onChange={this.onEncryptedChange}
                            readonly={this.state.progress}
                            onKeyDown={this.handleKeyPress} />
                    </Bem>
                    <Bem
                        key='decrypted-label'
                        block='service-utils'
                        elem='label'>
                        {i18n('service-page', 'service-utils-decrypted-link')}
                    </Bem>
                    <Bem
                        key='decrypted-value'
                        block='service-utils'
                        elem='value'>
                        <InputWithButton
                            theme='copy-input'
                            value={this.state.decrypted}
                            readonly
                            text={i18n('hints', 'link-copied-to-buffer')}
                            to='top-right'
                            timeout />
                    </Bem>
                    <Button
                        view='default'
                        tone='grey'
                        type='action'
                        size='n'
                        theme='action'
                        progress={this.state.progress}
                        onClick={this.onDecrypt}>
                        {i18n('common', 'decrypt')}
                    </Button>
                </Bem>
            </Bem>
        );
    }

    handleKeyPress(e) {
        if (e.key === 'Enter') {
            this.onDecrypt();
        }
    }

    setWrapperRef(wrapper) {
        this._wrapper = wrapper;
    }

    setEncryptedAnchor(anchor) {
        this._encryptedAnchor = anchor;
    }

    onEncryptedChange(nextEncrypted) {
        this.setState({
            encrypted: nextEncrypted
        });
    }

    onDecrypt() {
        const encryptedTrim = this.state.encrypted.trim();

        this.setState({
            decrypted: '',
            progress: true
        });

        serviceApi.decryptLink(this.props.service.id, encryptedTrim).then(response => {
            this.setState({
                decrypted: response.decrypted_url,
                progress: false,
                error: false
            });
        }, error => {
            this.setState({
                progress: false,
                error: error.message
            });
        });
    }
}

ServiceUtils.propTypes = {
    service: serviceType
};
