import {Request, Response} from '@yandex-data-ui/core/build/types';

export default (req: Request, res: Response) => {
    // Тут надо сделать не optional: https://github.yandex-team.ru/data-ui/ui-core-layout/blob/master/src/types.ts#L110
    return res.send(res.renderLayout2!({ // eslint-disable-line @typescript-eslint/no-non-null-assertion
        name: 'home',
        title: 'UICore Template App',
        nonce: req.nonce,
    }));
};
