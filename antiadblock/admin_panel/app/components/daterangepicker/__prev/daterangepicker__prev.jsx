import React from 'react';
import PropTypes from 'prop-types';

import {Th} from 'app/components/table/table-components';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import {dateLibDayJsType} from 'app/types';

export default class DaterangepickerPrev extends React.Component {
    handlePrev(calendar) {
        const {
            handlePrev
        } = this.props;

        if (handlePrev) {
            handlePrev(calendar);
        }
    }

    render() {
        const {
            prev,
            calendar
        } = this.props;

        const onClick = calendar && prev ? this.handlePrev.bind(this, calendar) : () => {};
        const prevProps = {
            onClick
        };

        return (
            <Th
                block='daterangepicker-table'
                elem='cell'
                mods={{
                    header: true,
                    prev: true,
                    available: prev
                }}
                {...prevProps}>
                {prev ? <Icon
                    glyph='type-arrow'
                    direction='left' /> : null}
            </Th>
        );
    }
}

DaterangepickerPrev.propTypes = {
    calendar: dateLibDayJsType.isRequired,
    prev: PropTypes.bool,
    handlePrev: PropTypes.func
};

DaterangepickerPrev.defaultProps = {
    prev: true
};
