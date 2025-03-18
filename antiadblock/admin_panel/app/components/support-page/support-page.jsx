import React from 'react';

import Bem from 'app/components/bem/bem';
import i18n from 'app/lib/i18n';

import Item from './__item/support-page__item';
import './support-page.css';

const contacts = [{
    key: 'telephone',
    content: '+7 (916) 364-16-14'
}, {
    key: 'telegram',
    content: '@antiadbsupport',
    link: '//telegram.me/antiadbsupport'
}, {
    key: 'email',
    content: 'antiadb-yndx@yandex.ru',
    link: 'mailto:antiadb-yndx@yandex.ru'
}];

const information = [{
    key: 'working-time',
    content: 'пн-пт c 10-00 до 19-00'
}, {
    key: 'lunch',
    content: 'с 13-00 до 14-00'
}];

export default class SupportPage extends React.Component {
    render() {
        return (
            <Bem block='support-page'>
                <Bem
                    block='support-page'
                    elem='header'>
                    {i18n('common', 'contacts')}
                </Bem>
                <Bem
                    block='support-page'
                    elem='contacts'>
                    {contacts.map(item => {
                        return (
                            <Item
                                key={item.key}
                                item={item} />
                        );
                    })}
                </Bem>
                <Bem
                    block='support-page'
                    elem='information-list'>
                    {information.map(item => {
                            return (
                                <Item
                                    key={item.key}
                                    item={item} />
                            );
                        })}
                </Bem>
            </Bem>
        );
    }
}
