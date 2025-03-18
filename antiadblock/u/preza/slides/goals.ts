import moment from "moment";
import PptxGenJS from "pptxgenjs";
import {getLastComment, getSTIssues} from "../utils/utils";
import { TITLE_FONT_SIZE, H1_FONT_SIZE } from '../const';

let week_start = moment().day('Monday').format('YYYY-MM-DD');
const PAGE_HEIGHT = 5.5 - 1; // 1 for title

console.log(week_start);
const UPDATED_QUERY = `Queue: ANTIADB Updated: >= ${week_start}`;

export async function* createSlides() {
    console.log('/--------------------------------------------/');
    console.log('START GOALS SLIDE');
    let issues: any[] = await getSTIssues(UPDATED_QUERY);
    let goalsIssues: any = {};

    console.log('MAKING GOALS SLIDE....');
    for (const issue of issues) {
        let goals: any[] = issue.getField('goals');
        if (goals && goals.length != 0) {
            for (const goal of goals) {
                let lastComment: any = await getLastComment(issue);
                let result: any = {
                    'status': issue.getStatus(),
                    'key': issue.getField('key'),
                    'summary': issue.getField('summary')
                };
                if (lastComment) {
                    let msg: string = lastComment.getText();
                    msg = msg.replace('[WEEKLY]', '').trim();
                    result['msg'] = msg;
                    result['author'] = lastComment.getCreatedBy().getDisplay();
                    let issuesArray: any[] = goalsIssues[goal.display];
                    if (!issuesArray) {
                        issuesArray = []
                        goalsIssues[goal.display] = issuesArray;
                    }
                    issuesArray.push(result);
                }
            }
        }
    }

    for (const goal of Object.keys(goalsIssues)) {
        const slide: PptxGenJS.Slide = yield;

        let issues: any[] = goalsIssues[goal];
        slide.addText(goal, { x: 0.5, y: 0.5, fontSize: TITLE_FONT_SIZE, color: '363636', fill: { color:'F1F1F1' } });

        let issuesByAuthor: any = {};
        for (const issue of issues) {
            let author: string = issue['author'];
            let result: string[] = issuesByAuthor[author];
            if (!result) {
                result = [];
                issuesByAuthor[author] = result;
            }
            result.push(issue);
        }
        let rowsCount = issues.length + Object.keys(issuesByAuthor).length;
        let step = Math.min(PAGE_HEIGHT / rowsCount, 0.3);
        const fontSize = 150 * step / 3;

        let offset = 1;
        for (const author of Object.keys(issuesByAuthor)) {
            let texts: any[] = [];
            for (const issue of issuesByAuthor[author]) {
                texts.push({text: `[${issue['status']}] ${issue['key']}: ${issue['summary']}`, options: {bullet: true, fontSize: fontSize, indentLevel: 0,}});
                texts.push({text: issue['msg'], options: {bullet: true, fontSize: fontSize, indentLevel: 1,}});
            }
            slide.addText(author, {x: 0.5, y: offset, bold: true, fontSize: H1_FONT_SIZE, valign: 'top'});
            slide.addText(texts, {x: 0.5, y: offset + 1.5 * step, valign: 'top'});
            offset += (texts.length + 2.5) * step;
        }
    }
    console.log('DONE GOALS');
    console.log('/--------------------------------------------/');
}
