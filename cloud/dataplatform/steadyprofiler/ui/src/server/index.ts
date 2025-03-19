import Core from '@yandex-data-ui/core';
import {MiddlewareStage} from '@yandex-data-ui/core/build/types';
import {createMiddleware} from '@yandex-data-ui/ui-core-layout';
const app = new Core({name: 'profiler-archive'});

export default app;
export const utils = app.utils;

if (app.devMode) {
    const sourceMaps = require('source-map-support');
    sourceMaps.install();
}

app.setupHealthcheck({
    path: '/ping',
    handler: (req, res) => {
        res.status(200).send({status: 'It is fine'});
    },
});

app.useMiddleware({
    stage: MiddlewareStage.AfterAuth,
    fn: createMiddleware(),
});

if (require.main === module) {
    app.run();
}
