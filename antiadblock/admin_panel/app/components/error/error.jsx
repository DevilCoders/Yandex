import React from 'react';
import PropTypes from 'prop-types';
import Header from 'app/components/header/header';

import Footer from 'lego-on-react/src/components/footer/footer.react';
import Copyright from 'lego-on-react/src/components/copyright/copyright.react';

import i18n from 'app/lib/i18n';
import './error.css';

export default class Error extends React.Component {
    render() {
        return (
            <div className='error'>
                <Header />
                <div className='error__content'>
                    <div className='error__status'>
                        {this.props.status ? this.props.status : 500}
                    </div>
                    <div className='error__message'>
                        {this.props.messageI18N ? i18n('error', this.props.messageI18N) : 'none'}
                    </div>
                </div>
                <Footer
                    tld='ru'
                    region='ru' >
                    <Footer.Column key='column1'>
                        <Footer.Link url='https://yandex.ru/company/'>{i18n('common', 'about')}</Footer.Link>
                        <Footer.Link url='/support'>{i18n('common', 'support')}</Footer.Link>
                    </Footer.Column>
                    <Footer.Column key='column2' side='right'>
                        <Copyright>
                            <Footer.Link url='https://yandex.ru'>{i18n('common', 'yandex')}</Footer.Link>
                        </Copyright>
                    </Footer.Column>
                </Footer>
            </div>
        );
    }
}

Error.propTypes = {
    status: PropTypes.number,
    messageI18N: PropTypes.string
};
