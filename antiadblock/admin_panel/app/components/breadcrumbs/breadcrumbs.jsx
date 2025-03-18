import React from 'react';
import PropTypes from 'prop-types';
import {Link} from 'react-router-dom';

import Bem from 'app/components/bem/bem';

import './breadcrumbs.css';

class Breadcrumbs extends React.Component {
    render() {
        const path = this.props.path || [];
        const current = path.pop() || {
            title: ''
        };

        return (
            <Bem block='breadcrumbs'>
                {path.map(item => {
                    return (
                        <Bem
                            key={item.title}
                            block='breadcrumbs'
                            elem='link'>
                            <Link
                                theme='pseudo'
                                to={item.url} >
                                {item.title}
                            </Link>
                        </Bem>
                    );
                })}
                <Bem
                    key={current.title}
                    block='breadcrumbs'
                    elem='current'>
                    {current.title}
                </Bem>
            </Bem>
        );
    }
}

Breadcrumbs.propTypes = {
    path: PropTypes.arrayOf(PropTypes.shape({
        title: PropTypes.string,
        url: PropTypes.string
    }))
};

export default Breadcrumbs;
