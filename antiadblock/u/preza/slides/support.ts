import pptxgen from "pptxgenjs";
import {getSTIssues, makeCall, getWeek} from "../utils/utils";
import { TITLE_FONT_SIZE } from '../const';

let week_start = getWeek().start.format('YYYY-MM-DD');


const RESOLVED_QUERY = `Queue: ANTIADBSUP Resolved: >= ${week_start}`;
const CREATED_QUERY = `Queue: ANTIADBSUP Created: >= ${week_start}`;

export async function* createSlide() {
    console.log('/--------------------------------------------/');
    console.log('START SUPPORT TICKETS SLIDE');
    console.log('GETTING SUPPORT ISSUES...');
    let resolved = await getSTIssues(RESOLVED_QUERY);
    let created = await getSTIssues(CREATED_QUERY);
    console.log(`CREATED: ${created.length}. RESOLVED: ${resolved.length}`);

    console.log('GETTING COOKIE ISSUES...');
    let resolved_cookie = await getSTIssues(RESOLVED_QUERY + ' Tags: cookie_rule');
    let created_cookie = await getSTIssues(CREATED_QUERY + ' Tags: cookie_rule');
    console.log(`CREATED: ${created_cookie.length}. RESOLVED: ${resolved_cookie.length}`);

    const slide: pptxgen.Slide = yield;

    // Tickets
    slide.addText('Дежурство (тикеты и потери)', {x: 1, y: 0.5, fontSize: TITLE_FONT_SIZE});
    slide.addText([
        {text: 'Тикеты', options: {fontSize: 28, bold: true}},
        {text: `Открыто (всего): ${created.length}`, options: {bullet: true, fontSize: 18}},
        {text: `Кука дня: ${created_cookie.length}`, options: {bullet: true, fontSize: 18, indentLevel: 1}},
        {text: `Закрыто (всего): ${resolved.length}`, options: {bullet: true, fontSize: 18}},
        {text: `Кука дня: ${resolved_cookie.length}`, options: {bullet: true, fontSize: 18, indentLevel: 1}}
    ], {x: 0.5, y: 2});

    // Потери
    console.log('FETCHING CHART...');
    let download_link = 'https://charts.yandex-team.ru/api/scr/v1/screenshots/preview/editor/r0z0ptfsc05in?_no_controls=true&__scr_width=1600&__scr_height=360&_embedded=1';    
    let image: string = await makeCall(download_link);
    console.log(`IMAGE SIZE (base64): ${image.length}`);
    slide.addImage({x: 0.0, y: 3.8, w: 10, h: 2, data: `data:image/png;base64,${image}`});
    let textboxText = "Потери в поддержке";
    let textboxOpts = { x: 0.1, y: 3.7, fontSize: 10, color: '363636', fill: { color:'F1F1F1' } };
    slide.addText(textboxText, textboxOpts);

    console.log('/--------------------------------------------/');
    return slide;
}
