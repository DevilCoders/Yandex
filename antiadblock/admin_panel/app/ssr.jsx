import Helmet from 'react-helmet';

const IS_PRODUCTION = (process.env.NODE_ENV === 'production');

// Prevent XSS
function printWindowData(data) {
    const dataSource = JSON.stringify(data);
    const escapedDataSource = dataSource.replace(/([<>/\u2028\u2029])/g, '\\$1');
    return escapedDataSource;
}

function renderDocument(helmet, appHtml, appState, appData) {
    return `<!doctype html>
<html>
    <head>
        <link rel="shortcut icon" href="//yastatic.net/iconostasis/_/8lFaTHLDzmsEZz-5XaQg9iTWZGE.png">
        ${helmet.title.toString()}
        ${helmet.meta.toString()}

        ${helmet.link.toString()}
        ${IS_PRODUCTION ?
            `<link rel="stylesheet" href="${appData.staticHost}/build/${appData.bundle}.${appData.lang}.build.css"/>` : ''}

        ${helmet.script.toString()}
        <base href="${appData.staticHost}/build/">
    </head>
    <body class="b-page">
        <div id="mount">${appHtml}</div>
        <script src="${appData.staticHost}/build/vendor.js"></script>
        <script src="${appData.staticHost}/build/${appData.bundle}.${appData.lang}.js"></script>
        <script nonce="${appData.nonce}">__init__(${printWindowData(appState)}, ${printWindowData(appData)});</script>
    </body>
</html>`;
}

/**
 * Server-side render
 * @param {string} bundleName    Name of bundle to render
 * @param {string} location      Location for router (e.g. req.url)
 * @param {*}      appData       Data passed to application
 * @returns {Promise}
 */
export default async function ssr(bundleName, location, appData) {
    const helmet = Helmet.renderStatic();

    return {
        html: renderDocument(helmet, '', {}, appData)
    };
}
