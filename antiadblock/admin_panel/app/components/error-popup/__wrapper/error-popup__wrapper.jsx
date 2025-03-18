import React from 'react';
import Bem from 'app/components/bem/bem';
import PropTypes from 'prop-types';

import Icon from 'lego-on-react/src/components/icon/icon.react';
import Link from 'lego-on-react/src/components/link/link.react';

import '../__attention-icon/error-popup__attention-icon.css';
import '../__close-icon/error-popup__close-icon.css';
import '../__content/error-popup__content.css';

import './error-popup__wrapper.css';
import 'app/components/icon/_theme/icon_theme_attention.css';
import 'app/components/icon/_theme/icon_theme_cross-white.css';
import 'app/components/icon/_opacity/icon_opacity_40.css';

import i18n from 'app/lib/i18n';

export default class ErrorPopupWrapper extends React.Component {
    render() {
        return (
            <Bem
                block='error-popup'
                elem='wrapper'>
                <Bem
                    key='content'
                    block='error-popup'
                    elem='content'>
                    <Bem
                        block='error-popup'
                        elem='content-message'>
                        {this.props.message}
                    </Bem>
                    <Bem
                        block='error-popup'
                        elem='content-try-again'>
                        {i18n('error', 'try-again-message')}
                    </Bem>
                </Bem>
                <Bem
                    key='info-icon'
                    block='error-popup'
                    elem='attention-icon'>
                    <Icon
                        size='s'
                        mix={[{
                            block: 'icon',
                            mods: {
                                theme: 'attention'
                            }
                        }]} />
                </Bem>
                <Bem
                    key='close-icon'
                    block='error-popup'
                    elem='close-icon'>
                    <Link
                        theme='pseudo'
                        onClick={this.props.onClose}>
                        <Icon
                            size='xs'
                            mix={[{
                                block: 'icon',
                                mods: {
                                    theme: 'cross-white'
                                }
                            }]} />
                    </Link>
                </Bem>
            </Bem>
        );
    }
}

ErrorPopupWrapper.propTypes = {
    message: PropTypes.string,
    onClose: PropTypes.func
};
