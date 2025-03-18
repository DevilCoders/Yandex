import React from 'react';
import {render} from 'enzyme';
import toJson from 'enzyme-to-json';

import ColorizedText from 'app/components/colorized-text/colorized-text';

describe('colorized-text component', () => {
    test('green color', () => {
        const wrapper = render(<ColorizedText color='green' />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });

    test('empty color', () => {
        const wrapper = render(<ColorizedText />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });
});
