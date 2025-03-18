import process from "process";
import https from "https";
import moment from "moment";

const ABC_MAIN_SUPPORT_ROLE_ID = "antiadb_main";
const ABC_ANTIADBLOCK_ID = 1526;

async function httpsRequest(url: string, token: string): Promise<Buffer> {
    return new Promise((resolve) => https.get(url, {
        headers: {
            Authorization: 'OAuth ' + token
        }
    }, function (res) {
        let data: Buffer[] = [];
        let dataLen = 0;

        res.on('data', function (chunk) {
            data.push(chunk);
            dataLen += chunk.length;

        }).on('end', function () {
            let buf = Buffer.alloc(dataLen);

            for (var i = 0, len = data.length, pos = 0; i < len; i++) {
                data[i].copy(buf, pos);
                pos += data[i].length;
            }

            resolve(buf);
        }).on('error', (error) => {
            console.error(error.message);
        });
    }));
}

export async function getDuty(): Promise<string | undefined> {
    // https://oauth.yandex-team.ru/authorize?response_type=token&client_id=23db397a10ae4fbcb1a7ab5896dc00f6
    let TOKEN = process.env.TOOLS_TOKEN as string;
    const buf = await httpsRequest(`https://abc-back.yandex-team.ru/api/v4/services/${ABC_ANTIADBLOCK_ID}/on_duty/`, TOKEN);
    const answer = JSON.parse(buf.toString());

    for (let i = 0; i < answer.length; i++) {
        const item = answer[i];
        if (item && item.schedule.slug === ABC_MAIN_SUPPORT_ROLE_ID) {
            return item.person.login as string;
        }
    }

    return undefined;
}

export async function makeCall(url: string): Promise<string> {
    // https://oauth.yandex-team.ru/authorize?response_type=token&client_id=09cea1cc285845b7b4dc3f409fcacad9
    const TOKEN = process.env.STAT_TOKEN as string;
    const buf = await httpsRequest(url, TOKEN);
    return buf.toString('base64');
}

export async function getSTIssues(query: string): Promise<any[]> {
    // https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b
    let TOKEN = process.env.STARTREK_TOKEN;
    var Client = require('@yandex-int/stapi'),
        client = new Client({
            entrypoint: 'https://st-api.yandex-team.ru',
            retries: 2,
            timeout: 2000
        }),
        session = client.createSession({
            token: TOKEN
        });
    session = client.createSession({
        token: TOKEN
    });

    let page: any = await new Promise((resolve) => session.issues.getAll({query: query}, function (error: any, page: any) {
        resolve(page);
    }));

    return await new Promise((resolve) => page.fetchAll(function (error: any, arr: any) {
        resolve(arr);
    }));
}

export async function getLastComment(issue: any) {
    let week_start_date = moment().day('Monday');

    return await new Promise((resolve) => issue.getComments(function (error: any, comments: any) {
        let result = null;
        for (const comment of comments.toArray()) {
            let createdAt: Date = comment.getCreatedAt();
            let text: string = comment.getText();
            if (createdAt.getTime() > week_start_date.toDate().getTime() && text.indexOf('[WEEKLY]') >= 0) {
                if (result == null || result.getCreatedAt().getTime() < createdAt.getTime()) {
                    result = comment;
                }
            }
        }
        resolve(result);
    }));
}

export function checkToken(token: string): asserts token
{
    if (process.env[token] === undefined) {
        console.log(`Env ${token} must be defined`);
        process.exit(1)
    }
}

export function getWeek() {
    if (moment().day() == 0) {
        return { start: moment().subtract(6, 'days'), end: moment()};
    }
    
    return {start: moment().startOf('week').subtract(1, 'days'), end: moment().endOf('week').subtract(1, 'days')};
}

export function getYear() {
    return moment().year();
}