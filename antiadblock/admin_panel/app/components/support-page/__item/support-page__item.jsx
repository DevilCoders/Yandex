import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import i18n from 'app/lib/i18n';

import './support-page__item.css';

export default class SupportPageItem extends React.Component {
    render() {
        const item = this.props.item;
        return (
            <Bem
                key={item.key}
                block='support-page-item'>
                <Bem
                    block='support-page-item'
                    elem='title'>
                    {i18n('common', item.key)}
                </Bem>
                <Bem
                    block='support-page-item'
                    elem='content'>
                    {item.link ?
                        <a className='link' href={item.link}>
                            {item.content}
                        </a> :
                        item.content
                    }
                </Bem>
            </Bem>
        );
    }
}

SupportPageItem.propTypes = {
    item: PropTypes.shape({
        key: PropTypes.string.isRequired,
        content: PropTypes.string.isRequired,
        link: PropTypes.string
    })
};
