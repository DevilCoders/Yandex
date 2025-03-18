import React from 'react';
import {shallow} from 'enzyme';
import toJson from 'enzyme-to-json';

import TagList from 'app/components/tag-list/tag-list';

describe('tag-list component', () => {
    test('render with props', () => {
        const wrapper = shallow(
            <TagList
                value={['value1', 'value2', 'value3']}
                placeholder='placeholder' />
        );
        expect(toJson(wrapper)).toMatchSnapshot();
    });

    test('render with readOnly=true', () => {
        const wrapper = shallow(
            <TagList
                value={['value1', 'value2', 'value3']}
                placeholder='placeholder'
                readOnly />
        );
        expect(toJson(wrapper)).toMatchSnapshot();
    });
});
