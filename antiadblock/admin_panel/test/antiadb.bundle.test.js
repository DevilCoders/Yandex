import render from '../app/ssr.jsx';
import Helmet from 'react-helmet';

const appData = {
    lang: 'ru',
    nonce: 'foobar',
    staticHost: './static',
    bundle: 'antiadb'
};

test('server-side rendering', async () => {
    // какой-то костыль - https://github.com/nfl/react-helmet/issues/203
    Helmet.canUseDOM = false;

    return render('index', '/', appData)
        .then(({html}) => {
            expect((html.indexOf('<script nonce="foobar">') > -1)).toBeTruthy();
        });
});
