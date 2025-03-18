import React from 'react';
import {shallow} from 'enzyme';
import toJson from 'enzyme-to-json';

import StatusCircle from 'app/components/status-circle/status-circle';

describe('status-circle component', () => {
    test('active status', () => {
        const wrapper = shallow(<StatusCircle status='active' />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });

    test('empty status', () => {
        const wrapper = shallow(<StatusCircle />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });
});
