import pptxgen from "pptxgenjs";
import moment from "moment";
import {getSTIssues, getDuty, getLastComment, makeCall} from '../utils/utils';
import PptxGenJS from "pptxgenjs";
import { H1_FONT_SIZE, TEXT_FONT_SIZE, ANTIADB_TEAM, ANTIADB_QUEUES } from '../const';

const PAGE_HEIGHT = 5.5 - 1; // 1 for title

let week_start = moment().day('Monday').format('YYYY-MM-DD');

console.log(week_start)

const UPDATED_QUERY = `assignee: ${ANTIADB_TEAM.join(',')} updated: >= ${week_start} and queue: ${ANTIADB_QUEUES.join(', ')} and Type: ! Release`;

type Issue = {
    assignee: string,
    goals: string,
    summary: string,
    key: string,
    issue: any
}

let duty: string | undefined;
let issues: any;

export function createDutySlide() {
    return createSlides(true);
}

export function createNoGoalsSlides() {
    return createSlides(false);
}

export async function* createSlides(isDuty: boolean) {
    console.log('/--------------------------------------------/');
    console.log('START NO GOALS SLIDE');

    console.log('FETCHING DUTY...');
    duty = duty || await getDuty();
    console.log(`DUTY: ${duty}`);

    console.log(`FETCHING ISSUES. QUERY: ${UPDATED_QUERY}`);
    issues = issues || await getSTIssues(UPDATED_QUERY);
    console.log(`${issues.length} ISSUES FOUND`);

    console.log('MAKING SLIDE....');
    const filtredIssues = issues.filter((element: any) => {
        const goals = element.getField('goals');
        const queue = element.getField('queue');
        if (element.getField('assignee').id == duty)
            return isDuty;
        else
            // Цели сейчас беруться только из очереди ANTIADB. Возможно это неправильно, но как есть.
            // Проверка queue нужна чтобы не удалять тикеты из других очередей
            return !isDuty && ((!goals || !goals.length)|| (queue && queue.key !== 'ANTIADB'));
    });

    filtredIssues.sort((a: any, b: any) => {
        const aId = a.getField('assignee').id;
        const bId = b.getField('assignee').id;
        if (aId == duty) {
            return -1;
        } else if (bId == duty) {
            return 1;
        }
        return 0;
    });

    const issuesMap = filtredIssues.reduce((acc: Record<string, Issue[]>, element: any) => {
        const name = element.getField('assignee').display;
        acc[name] = acc[name] || [];
        acc[name].push({
            assignee: name,
            goals: JSON.stringify(element.getField('goals')),
            summary: element.getField('summary'),
            key: element.getField('key'),
            issue: element
        });
        return acc;
    }, {});

    for (const user in issuesMap) {
        if (issuesMap.hasOwnProperty(user)) {
            const arr: pptxgen.TextProps[] = [];
            for (const element of issuesMap[user]) {
                let lastComment: any = await getLastComment(element.issue);
                if (lastComment) {
                    let msg: string = lastComment.getText().replace('[WEEKLY]', '').trim();
                    if (msg.length > 0) {
                        console.log(`weekly: ${msg}`);
                        arr.push({
                            text: `${element.key}: ${element.summary}`,
                            options: {bullet: true}
                        });
                        arr.push({
                            text: msg,
                            options: {bullet: true, indentLevel: 1}
                        });
                    }
                }
            }

            if (!arr.length) {
                continue;
            }

            // Рассчитываем fontSize
            const size = arr.length;
            const step = Math.min(PAGE_HEIGHT / size, 0.3);
            const fontSize = 150 * step / 3;

            let assignee_id: string = issuesMap[user][0].issue.getField('assignee').id;
            let avatar_url: string = `https://center.yandex-team.ru/api/v1/user/${assignee_id}/photo/460/square.jpg`
            let avatar: string = await makeCall(avatar_url);
            let slide: PptxGenJS.Slide = yield;
            slide.addText(user, {x: 0.5, y: 0.5, fontSize: H1_FONT_SIZE, bold: true});
            slide.addText(arr, {x: 0.5, y: 1, valign: 'top', fontSize: fontSize});
            slide.addImage({data: `data:image/png;base64,${avatar}`, w: 2, h: 2, x: 8});
        }
    }
    console.log('DONE');
    console.log('/--------------------------------------------/');
}
