import React from 'react';
import PropTypes from 'prop-types';
import {Link} from 'react-router-dom';

import './service-page__menu.css';
import {antiadbUrl} from 'app/lib/url';
import i18n from 'app/lib/i18n';

export default class ServicePageMenu extends React.Component {
    render() {
        const baseUrl = antiadbUrl(`/service/${this.props.service.id}`);

        let links = this.props.links.map(action => ({
            key: action,
            title: i18n('service-page', `menu-${action}`),
            url: `${baseUrl}/${action}`
        }));

        return (
            <div className='service-page__menu'>
                {links.map(item => (
                    <Link
                        key={item.key}
                        to={item.url}>
                        <div className={`service-page__menu-link ${this.props.active === item.key ? 'service-page__menu-link_active' : ''}`}>
                            {item.title}
                        </div>
                    </Link>
                ))}
            </div>
        );
    }
}

ServicePageMenu.propTypes = {
    service: PropTypes.object.isRequired,
    active: PropTypes.string,
    links: PropTypes.array
};
