const { CssFilter } = adguard.rules;

const genericHide = CssFilter.RETRIEVE_TRADITIONAL_CSS + CssFilter.RETRIEVE_EXTCSS + CssFilter.GENERIC_HIDE_APPLIED;

QUnit.test('Css Filter Rule', (assert) => {
    const ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com##.sponsored';
    const rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.notOk(rule.whiteListRule);
    assert.notOk(rule.extendedCss);
    assert.equal(5, rule.getRestrictedDomains().length);
    assert.ok(rule.getRestrictedDomains().indexOf('gamespot.com') >= 0);
    assert.ok(rule.getRestrictedDomains().indexOf('mint.com') >= 0);
    assert.ok(rule.getRestrictedDomains().indexOf('slidetoplay.com') >= 0);
    assert.ok(rule.getRestrictedDomains().indexOf('smh.com.au') >= 0);
    assert.ok(rule.getRestrictedDomains().indexOf('zattoo.com') >= 0);
    assert.equal('.sponsored', rule.cssSelector);
});

QUnit.test('Elemhide Rules As CSS', (assert) => {
    function tryTest(rule) {
        try {
            new adguard.rules.CssFilterRule(rule);
            throw new Error('Rule should not be parsed successfully');
        } catch (ex) {
            assert.equal(ex.message, `Invalid elemhide rule: ${rule}`);
        }
    }

    let selector = 'img[title|={]';
    let ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = 'body { background: red!important; }';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = 'body { background: red!important; }';
    ruleText = `example.org#@#${selector}`;
    tryTest(ruleText);

    selector = 'a[title=\"\\\"\"]{background:url()}';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = 'body\\{\\}, body { background: lightblue url(\"https://www.w3schools.com/cssref/img_tree.gif\") no-repeat fixed center!important; }';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = 'body /*({})*/ { background: lightblue url(\"https://www.w3schools.com/cssref/img_tree.gif\") no-repeat fixed center!important; }';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = 'body /*({*/ { background: lightblue url(\"https://www.w3schools.com/cssref/img_tree.gif\") no-repeat fixed center!important; }';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = '.generic1 /*comment*/';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = '\\\\/*[*/, body { background: lightblue url(\"https://www.w3schools.com/cssref/img_tree.gif\") no-repeat fixed center!important; } ,\\/*]';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = 'body:not(blabla/*[*/) { background: lightblue url(\"https://www.w3schools.com/cssref/img_tree.gif\") no-repeat fixed center!important; } /*]*\\/';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);

    selector = 'a //';
    ruleText = `example.org##${selector}`;
    tryTest(ruleText);
});

QUnit.test('Css Filter Rule Extended Css', (assert) => {
    let ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com##.sponsored[-ext-contains=test]';
    let rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.notOk(rule.whiteListRule);
    assert.ok(rule.extendedCss);
    assert.equal('.sponsored[-ext-contains=test]', rule.cssSelector);

    ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com##.sponsored[-ext-has=test]';
    rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.notOk(rule.whiteListRule);
    assert.ok(rule.extendedCss);
    assert.equal('.sponsored[-ext-has=test]', rule.cssSelector);

    ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com##.sponsored:has(test)';
    rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.notOk(rule.whiteListRule);
    assert.ok(rule.extendedCss);
    assert.equal('.sponsored:has(test)', rule.cssSelector);

    ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#?#div';
    rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.notOk(rule.whiteListRule);
    assert.notOk(rule.isInjectRule);
    assert.ok(rule.extendedCss);
    assert.equal('div', rule.cssSelector);

    ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#$?#div { background-color: #333!important; }';
    rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.notOk(rule.whiteListRule);
    assert.ok(rule.isInjectRule);
    assert.ok(rule.extendedCss);
    assert.equal('div { background-color: #333!important; }', rule.cssSelector);
});

QUnit.test('Css Filter WhiteList Rule', (assert) => {
    let ruleText = 'gamespot.com,mint.com#@#.sponsored';
    let rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getPermittedDomains().length > 0);
    assert.ok(rule.whiteListRule);
    assert.equal(2, rule.getPermittedDomains().length);
    assert.ok(rule.getPermittedDomains().indexOf('gamespot.com') >= 0);
    assert.ok(rule.getPermittedDomains().indexOf('mint.com') >= 0);
    assert.equal('.sponsored', rule.cssSelector);

    ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#@?#div';
    rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.ok(rule.whiteListRule);
    assert.notOk(rule.isInjectRule);
    assert.ok(rule.extendedCss);
    assert.equal('div', rule.cssSelector);

    ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#@$?#div { background-color: #333!important; }';
    rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0);
    assert.ok(rule.whiteListRule);
    assert.ok(rule.isInjectRule);
    assert.ok(rule.extendedCss);
    assert.equal('div { background-color: #333!important; }', rule.cssSelector);
});

QUnit.test('Css Exception Rules', (assert) => {
    const rule = new adguard.rules.CssFilterRule('##.sponsored');
    const rule1 = new adguard.rules.CssFilterRule('adguard.com#@#.sponsored');
    const filter = new adguard.rules.CssFilter([rule]);

    let { css } = filter.buildCss('adguard.com');
    let commonCss = filter.buildCss(null).css;
    assert.equal(css[0], commonCss[0]);

    filter.addRule(rule1);
    css = filter.buildCss('adguard.com').css;
    commonCss = filter.buildCss(null).css;
    assert.equal(css.length, 0);
    assert.equal(css.length, commonCss.length);

    css = filter.buildCss('another.domain').css;
    assert.ok(css.length > 0);

    filter.removeRule(rule1);
    css = filter.buildCss('adguard.com').css;
    commonCss = filter.buildCss(null).css;
    assert.ok(css.length > 0);
    assert.equal(css[0], commonCss[0]);
});

QUnit.module('CSS generic exception rules', () => {
    QUnit.test('Css plain generic and wildcard generic exception rules should work', (assert) => {
        const genericAllowlistingRule = new adguard.rules.CssFilterRule('#@#.adsbygoogle');
        const genericWildcardAllowlistingRule = new adguard.rules.CssFilterRule('*#@#.adsbygoogle');

        const specificBlockingRule = new adguard.rules.CssFilterRule('example.org##.adsbygoogle');
        const genericBlockingRule = new adguard.rules.CssFilterRule('##.adsbygoogle');

        const cssFilter = new adguard.rules.CssFilter([genericBlockingRule, specificBlockingRule]);
        const cssWildcardFilter = new adguard.rules.CssFilter([genericBlockingRule, specificBlockingRule]);

        const css = cssFilter.buildCss('example.org');
        const cssWildcard = cssWildcardFilter.buildCss('example.org');

        assert.ok(css.css.length > 0);
        assert.deepEqual(css, cssWildcard);

        cssFilter.addRule(genericAllowlistingRule);
        cssWildcardFilter.addRule(genericWildcardAllowlistingRule);

        const css2 = cssFilter.buildCss('example.org');
        const cssWildcard2 = cssWildcardFilter.buildCss('example.org');

        assert.ok(css2.css.length === 0);
        assert.deepEqual(css2, cssWildcard2);
    });

    QUnit.test('Work correctly for multiple rules', (assert) => {
        const otherSpecificBlockingRule = new adguard.rules.CssFilterRule('example.org##.ads1');
        const otherGenericAllowlistingRule = new adguard.rules.CssFilterRule('#@#.ads1');

        const genericAllowlistingRule = new adguard.rules.CssFilterRule('#@#.ads2');
        const specificBlockingRule = new adguard.rules.CssFilterRule('example.org##.ads2');

        const cssFilter = new adguard.rules.CssFilter([
            otherSpecificBlockingRule,
            otherGenericAllowlistingRule,
            specificBlockingRule,
            genericAllowlistingRule,
        ]);

        const css = cssFilter.buildCss('example.org');
        assert.ok(css.css.length === 0);
    });
});


QUnit.test('Css GenericHide Exception Rules', (assert) => {
    const genericOne = new adguard.rules.CssFilterRule('##.generic-one');
    const genericTwo = new adguard.rules.CssFilterRule('~google.com,~yahoo.com###generic');
    const nonGeneric = new adguard.rules.CssFilterRule('adguard.com##.non-generic');
    const injectRule = new adguard.rules.CssFilterRule('adguard.com#$#body { background-color: #111!important; }');
    const exceptionRule = new adguard.rules.CssFilterRule('adguard.com#@#.generic-one');
    const genericHideRule = new adguard.rules.UrlFilterRule('@@||adguard.com^$generichide');
    const elemHideRule = new adguard.rules.UrlFilterRule('@@||adguard.com^$elemhide');
    const filter = new adguard.rules.CssFilter([genericOne]);

    let { css } = filter.buildCss('adguard.com');
    let commonCss = filter.buildCss(null).css;
    assert.equal(css[0], commonCss[0]);

    filter.addRule(genericTwo);
    css = filter.buildCss('adguard.com').css;
    commonCss = filter.buildCss(null).css;
    assert.equal(css.length, 2);
    assert.equal(commonCss.length, 1);
    assert.equal(css[0], commonCss[0]);

    filter.addRule(nonGeneric);
    css = filter.buildCss('adguard.com').css;
    commonCss = filter.buildCss(null).css;
    assert.equal(css.length, 2);
    assert.equal(css[0], commonCss[0]);

    let otherCss = filter.buildCss('another.domain').css;
    assert.equal(otherCss.length, 2);

    filter.addRule(exceptionRule);
    css = filter.buildCss('adguard.com').css;
    commonCss = filter.buildCss(null).css;
    otherCss = filter.buildCss('another.domain').css;
    assert.equal(css.length, 1);
    assert.equal(commonCss.length, 0);
    assert.equal(otherCss.length, 1);

    filter.addRule(genericHideRule);
    css = filter.buildCss('adguard.com', genericHide).css;
    commonCss = filter.buildCss(null).css;
    otherCss = filter.buildCss('another.domain').css;
    assert.equal(css.length, 1);
    assert.ok(css[0].indexOf('#generic') < 0);
    assert.equal(commonCss.length, 0);
    assert.equal(otherCss.length, 1);

    filter.removeRule(exceptionRule);
    css = filter.buildCss('adguard.com', genericHide).css;
    commonCss = filter.buildCss(null).css;
    otherCss = filter.buildCss('another.domain').css;
    assert.equal(css.length, 1);
    assert.equal(commonCss.length, 1);
    assert.equal(otherCss.length, 2);

    filter.addRule(elemHideRule);
    css = filter.buildCss('adguard.com', genericHide).css;
    commonCss = filter.buildCss(null).css;
    otherCss = filter.buildCss('another.domain').css;
    assert.equal(css.length, 1);
    assert.equal(commonCss.length, 1);
    assert.equal(otherCss.length, 2);

    filter.addRule(injectRule);
    css = filter.buildCss('adguard.com', genericHide).css;
    commonCss = filter.buildCss(null).css;
    otherCss = filter.buildCss('another.domain').css;
    assert.equal(css.length, 2);
    assert.equal(commonCss.length, 1);
    assert.equal(otherCss.length, 2);
});

QUnit.test('Ublock Css Injection Syntax Support', (assert) => {
    let ruleText = 'yandex.ru##body:style(background:inherit;)';
    let cssFilterRule = adguard.rules.builder.createRule(ruleText, 0);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.convertedRuleText, 'yandex.ru#$#body { background:inherit; }');
    assert.ok(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(cssFilterRule.cssSelector, 'body { background:inherit; }');

    ruleText = 'yandex.ru#@#body:style(background:inherit;)';
    cssFilterRule = adguard.rules.builder.createRule(ruleText, 0);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.convertedRuleText, 'yandex.ru#@$#body { background:inherit; }');
    assert.ok(cssFilterRule.isInjectRule);
    assert.ok(cssFilterRule.whiteListRule);
    assert.equal(cssFilterRule.cssSelector, 'body { background:inherit; }');

    ruleText = 'yandex.ru##a[src^="http://domain.com"]';
    cssFilterRule = adguard.rules.builder.createRule(ruleText, 0);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(cssFilterRule.cssSelector, 'a[src^="http://domain.com"]');

    ruleText = "yandex.ru##[role='main']:style(display: none;)";
    cssFilterRule = adguard.rules.builder.createRule(ruleText, 0);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.convertedRuleText, 'yandex.ru#$#[role=\'main\'] { display: none; }');
    assert.ok(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(cssFilterRule.cssSelector, "[role='main'] { display: none; }");

    ruleText = 'example.com##a[target="_blank"][href^="http://api.taboola.com/"]:style(display: none;)';
    cssFilterRule = adguard.rules.builder.createRule(ruleText, 0);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.convertedRuleText, 'example.com#$#a[target="_blank"][href^="http://api.taboola.com/"] { display: none; }');
    assert.equal(cssFilterRule.cssSelector, 'a[target="_blank"][href^="http://api.taboola.com/"] { display: none; }');
});

QUnit.test('Some Complex Selector Rules', (assert) => {
    let ruleText = 'example.com##td[valign="top"] > .mainmenu[style="padding:10px 0 0 0 !important;"]';
    let cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, 'td[valign="top"] > .mainmenu[style="padding:10px 0 0 0 !important;"]');

    ruleText = 'example.com##a[target="_blank"][href^="http://api.taboola.com/"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, 'a[target="_blank"][href^="http://api.taboola.com/"]');

    ruleText = 'example.com###mz[width="100%"][valign="top"][style="padding:20px 30px 30px 30px;"] + td[width="150"][valign="top"][style="padding:6px 10px 0px 0px;"]:last-child';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, '#mz[width="100%"][valign="top"][style="padding:20px 30px 30px 30px;"] + td[width="150"][valign="top"][style="padding:6px 10px 0px 0px;"]:last-child');

    ruleText = 'example.com###st-flash > div[class] > [style^="width:"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, '#st-flash > div[class] > [style^="width:"]');

    ruleText = 'example.com##.stream-item[data-item-type="tweet"][data-item-id*=":"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, '.stream-item[data-item-type="tweet"][data-item-id*=":"]');

    ruleText = 'example.com##[class][style*="data:image"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, '[class][style*="data:image"]');

    ruleText = 'example.com##[onclick] > a[href^="javascript:"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, '[onclick] > a[href^="javascript:"]');

    ruleText = 'example.com###main_content_wrap > table[width="100%"] > tbody > tr > td:empty+td > aside > a[href="#"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, '#main_content_wrap > table[width=\"100%\"] > tbody > tr > td:empty+td > aside > a[href=\"#\"]');

    ruleText = 'pornolab.biz,pornolab.cc,pornolab.net###main_content_wrap > table[width="100%"] > tbody > tr > td:empty+td > aside > a[href="#"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);

    assert.equal(cssFilterRule.cssSelector, '#main_content_wrap > table[width="100%"] > tbody > tr > td:empty+td > aside > a[href="#"]');
    assert.equal(cssFilterRule.ruleText, ruleText);

    ruleText = 'm.hindustantimes.com##.container-fluid > section.noBorder[-ext-has=">.recommended-story>.recommended-list>ul>li:not([class]):empty"]';
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);

    assert.equal(cssFilterRule.ruleText, ruleText);
    assert.equal(cssFilterRule.cssSelector, '.container-fluid > section.noBorder[-ext-has=\">.recommended-story>.recommended-list>ul>li:not([class]):empty\"]');
});

QUnit.test('Invalid Css Injection Rules', (assert) => {
    try {
        // Invalid rule - lacking of css style
        var ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#$?#div';
        new adguard.rules.CssFilterRule(ruleText);
        throw new Error('Rule should not be parsed successfully');
    } catch (ex) {
        assert.equal(ex.message, 'Invalid css injection rule, no style presented: ~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#$?#div');
    }

    try {
        // Invalid rule - lacking of css style
        var ruleText = '~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#$?#div {asdasd]';
        new adguard.rules.CssFilterRule(ruleText);
        throw new Error('Rule should not be parsed successfully');
    } catch (ex) {
        assert.equal(ex.message, 'Invalid css injection rule, no style presented: ~gamespot.com,~mint.com,~slidetoplay.com,~smh.com.au,~zattoo.com#$?#div {asdasd]');
    }
});

QUnit.test('Valid Pseudo Class', (assert) => {
    let selector = '#main > table.w3-table-all.notranslate:first-child > tbody > tr:nth-child(17) > td.notranslate:nth-child(2)';
    let ruleText = `w3schools.com##${selector}`;
    let cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '#:root div.ads';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = "#body div[attr='test']:first-child  div";
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe::after';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe:matches-css(display: block)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe:matches-css-before(display: block)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe:matches-css-after(display: block)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe:has(.banner)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe:contains(test)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe:if(test)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe:if-not(test)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);
});

QUnit.test('Filter Rule With Colon', (assert) => {
    let selector = 'a[href^="https://w3schools.com"]';
    let ruleText = `w3schools.com##${selector}`;
    let cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '#Meebo\\:AdElement\\.Root';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule != null);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);
});

QUnit.test('Invalid Pseudo Class', (assert) => {
    let selector;
    let ruleText;

    selector = 'test:matches(.whatisthis)';
    try {
        ruleText = `yandex.ru##${selector}`;
        new adguard.rules.CssFilterRule(ruleText);
        throw new Error('Rule should not be parsed successfully');
    } catch (ex) {
        assert.equal(ex.message, `Unknown pseudo class: ${selector}`);
    }

    selector = '.todaystripe:properties(background-color: rgb\(0, 0, 0\))';
    try {
        ruleText = `w3schools.com##${selector}`;
        new adguard.rules.CssFilterRule(ruleText);
        throw new Error('Rule should not be parsed successfully');
    } catch (ex) {
        assert.equal(ex.message, `Unknown pseudo class: ${selector}`);
    }

    selector = '.todaystripe:-abp-properties(background-color: rgb\(0, 0, 0\))';
    try {
        ruleText = `w3schools.com##${selector}`;
        new adguard.rules.CssFilterRule(ruleText);
        throw new Error('Rule should not be parsed successfully');
    } catch (ex) {
        assert.equal(ex.message, `Unknown pseudo class: ${selector}`);
    }
});

QUnit.test('Extended Css Rules Pseudo Classes', (assert) => {
    let selector; let ruleText; let
        cssFilterRule;

    // :contains
    selector = '.todaystripe:contains(test)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe[-ext-contains="test"]';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    // :has
    selector = '.todaystripe:has(.banner)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe[-ext-has=".banner"]';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    // :matches-css
    selector = '.todaystripe:matches-css(display: block)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe[-ext-matches-css="display: block"]';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    // :matches-css-before
    selector = '.todaystripe:matches-css-before(display: block)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe[-ext-matches-css-before="display: block"]';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    // :matches-css-after
    selector = '.todaystripe:matches-css-after(display: block)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    // :if
    selector = '.todaystripe:if(.banner)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    // :if-not
    selector = '.todaystripe:if-not(.banner)';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);

    selector = '.todaystripe[-ext-matches-css-after="display: block"]';
    ruleText = `w3schools.com##${selector}`;
    cssFilterRule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(cssFilterRule);
    assert.ok(cssFilterRule.extendedCss);
    assert.notOk(cssFilterRule.isInjectRule);
    assert.notOk(cssFilterRule.whiteListRule);
    assert.equal(selector, cssFilterRule.cssSelector);
});

QUnit.test('Extended Css Build', (assert) => {
    const rule = new adguard.rules.CssFilterRule('adguard.com##.sponsored');
    const genericRule = new adguard.rules.CssFilterRule('##.banner');
    const extendedCssRule = new adguard.rules.CssFilterRule('adguard.com##.sponsored[-ext-contains=test]');
    const filter = new adguard.rules.CssFilter([rule, genericRule, extendedCssRule]);

    let selectors; let css; let extendedCss; let
        commonCss;

    selectors = filter.buildCss('adguard.com');
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(css.length, 2);
    assert.equal(extendedCss.length, 1);

    selectors = filter.buildCss('adguard.com', genericHide);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(css.length, 1);
    assert.equal(extendedCss.length, 1);
});

QUnit.test('Extended Css Build Common Extended', (assert) => {
    const rule = new adguard.rules.CssFilterRule('adguard.com##.sponsored');
    const genericRule = new adguard.rules.CssFilterRule('##.banner');
    const extendedCssRule = new adguard.rules.CssFilterRule('##.sponsored[-ext-contains=test]');
    const filter = new adguard.rules.CssFilter([rule, genericRule, extendedCssRule]);

    let selectors; let css; let extendedCss; let
        commonCss;

    selectors = filter.buildCss('adguard.com');
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(css.length, 2);
    assert.equal(extendedCss.length, 1);

    selectors = filter.buildCss('adguard.com', genericHide);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(css.length, 1);
    assert.equal(extendedCss.length, 0);
});

QUnit.test('Extended Css Build CssHits', (assert) => {
    const rule = new adguard.rules.CssFilterRule('adguard.com##.sponsored', 1);
    const genericRule = new adguard.rules.CssFilterRule('##.banner', 2);
    const extendedCssRule = new adguard.rules.CssFilterRule('adguard.com##.sponsored[-ext-contains=test]', 1);
    const filter = new adguard.rules.CssFilter([rule, genericRule, extendedCssRule]);

    let selectors; let css; let extendedCss; let
        commonCss;

    selectors = filter.buildCssHits('adguard.com');
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCssHits(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(commonCss[0].trim(), ".banner { display: none!important; content: 'adguard2%3B%23%23.banner' !important;}");
    assert.equal(css.length, 2);
    assert.equal(css[0].trim(), ".banner { display: none!important; content: 'adguard2%3B%23%23.banner' !important;}");
    assert.equal(css[1].trim(), ".sponsored { display: none!important; content: 'adguard1%3Badguard.com%23%23.sponsored' !important;}");
    assert.equal(extendedCss.length, 1);
    assert.equal(extendedCss[0].trim(), ".sponsored[-ext-contains=test] { display: none!important; content: 'adguard1%3Badguard.com%23%23.sponsored%5B-ext-contains%3Dtest%5D' !important;}");

    selectors = filter.buildCssHits('adguard.com', genericHide);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCssHits(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(commonCss[0].trim(), ".banner { display: none!important; content: 'adguard2%3B%23%23.banner' !important;}");
    assert.equal(css.length, 1);
    assert.equal(css[0].trim(), ".sponsored { display: none!important; content: 'adguard1%3Badguard.com%23%23.sponsored' !important;}");
    assert.equal(extendedCss.length, 1);
    assert.equal(extendedCss[0].trim(), ".sponsored[-ext-contains=test] { display: none!important; content: 'adguard1%3Badguard.com%23%23.sponsored%5B-ext-contains%3Dtest%5D' !important;}");
});

QUnit.test('Extended Css Build Inject Css', (assert) => {
    const rule = adguard.rules.builder.createRule('adguard.com##.sponsored', 0);
    const genericRule = new adguard.rules.CssFilterRule('##.banner');
    const extendedCssRule = new adguard.rules.CssFilterRule('adguard.com##.sponsored[-ext-contains=test]');
    const filter = new adguard.rules.CssFilter([rule, genericRule, extendedCssRule]);

    let selectors; let css; let extendedCss; let
        commonCss;

    const baseOption = adguard.rules.CssFilter.RETRIEVE_TRADITIONAL_CSS + adguard.rules.CssFilter.RETRIEVE_EXTCSS + adguard.rules.CssFilter.CSS_INJECTION_ONLY;

    const injectRule = adguard.rules.builder.createRule('adguard.com##body:style(background:inherit;)', 0);
    assert.ok(injectRule.isInjectRule);
    assert.notOk(injectRule.extendedCss);
    filter.addRule(injectRule);

    selectors = filter.buildCss('adguard.com', baseOption);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(css.length, 1);
    assert.equal(extendedCss.length, 1);

    selectors = filter.buildCss('adguard.com', baseOption + adguard.rules.CssFilter.GENERIC_HIDE_APPLIED);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(css.length, 1);
    assert.equal(extendedCss.length, 1);

    const exceptionInjectRule = new adguard.rules.CssFilterRule('adguard.com#@$#.sponsored { display: none!important;}');
    assert.ok(exceptionInjectRule.isInjectRule);
    assert.notOk(exceptionInjectRule.extendedCss);
    assert.ok(exceptionInjectRule.whiteListRule);
    filter.addRule(exceptionInjectRule);

    selectors = filter.buildCss('adguard.com', baseOption);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 1);
    assert.equal(css.length, 1);
    assert.equal(extendedCss.length, 1);
});

QUnit.test('Extended Css Selector Inject Rule', (assert) => {
    const rule = new adguard.rules.CssFilterRule('adguard.com##.bannerSponsor');
    const genericRule = new adguard.rules.CssFilterRule('##.banner');
    const extendedCssRule = new adguard.rules.CssFilterRule('adguard.com##.sponsored[-ext-contains=test]');
    const filter = new adguard.rules.CssFilter([rule, genericRule, extendedCssRule]);

    let selectors; let css; let extendedCss; let
        commonCss;

    let injectRule = new adguard.rules.CssFilterRule('adguard.com#$#.first-item h2:has(time) { font-size: 128px; }');
    assert.ok(injectRule.isInjectRule);
    assert.ok(injectRule.extendedCss);
    filter.addRule(injectRule);

    injectRule = new adguard.rules.CssFilterRule('#$#.first-item { font-size: 130px; }');
    assert.ok(injectRule.isInjectRule);
    filter.addRule(injectRule);

    selectors = filter.buildCss('adguard.com', adguard.rules.CssFilter.RETRIEVE_TRADITIONAL_CSS + adguard.rules.CssFilter.RETRIEVE_EXTCSS + adguard.rules.CssFilter.CSS_INJECTION_ONLY);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 2);
    assert.equal(css.length, 1);
    assert.equal(extendedCss.length, 2);

    selectors = filter.buildCss('adguard.com', adguard.rules.CssFilter.RETRIEVE_TRADITIONAL_CSS + adguard.rules.CssFilter.RETRIEVE_EXTCSS + adguard.rules.CssFilter.CSS_INJECTION_ONLY + adguard.rules.CssFilter.GENERIC_HIDE_APPLIED);
    css = selectors.css;
    extendedCss = selectors.extendedCss;
    commonCss = filter.buildCss(null).css;
    assert.equal(commonCss.length, 2);
    assert.equal(css.length, 0);
    assert.equal(extendedCss.length, 2);
});

QUnit.test('Css Filter WWW Test', (assert) => {
    const ruleText = 'www.google.com##body';
    const rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule != null);
    assert.equal(rule.permittedDomain, 'www.google.com');
});

QUnit.test('Permitted/Restricted domains Test', (assert) => {
    const ruleText = '##body';
    const rule = new adguard.rules.CssFilterRule(ruleText);

    rule.setRestrictedDomains(['lenta.ru']);
    rule.setRestrictedDomains(['lenta.ru', 'google.com']);

    assert.ok(rule != null);
    assert.equal(rule.getRestrictedDomains()[0], 'lenta.ru');
    assert.equal(rule.getRestrictedDomains()[1], 'google.com');
});

QUnit.test('Test wildcard domains in the css rules', (assert) => {
    const ruleText = 'google.*##body';
    const rule = new adguard.rules.CssFilterRule(ruleText);
    assert.ok(rule !== null);
    assert.ok(rule.isPermitted('google.com'));
    assert.ok(rule.isPermitted('google.de'));
    assert.ok(rule.isPermitted('google.co.uk'));
    assert.notOk(rule.isPermitted('google.eu.uk')); // non-existent tld

    const ruleText2 = '~yandex.*,google.*,youtube.*###ad-iframe';
    const rule2 = new adguard.rules.CssFilterRule(ruleText2);
    assert.ok(rule2 !== null);
    assert.ok(rule2.isPermitted('google.com'));
    assert.ok(rule2.isPermitted('youtube.ru'));
    assert.ok(rule2.isPermitted('youtube.co.id'));
    assert.notOk(rule2.isPermitted('yandex.com'));
    assert.notOk(rule2.isPermitted('www.yandex.ru'));
    assert.notOk(rule2.isPermitted('www.adguard.com'));
});

QUnit.test('Test inject rules containing url in the css content', (assert) => {
    const ruleText = 'example.com#$#body { background: url(http://example.org/empty.gif) }';
    assert.throws(() => {
        // eslint-disable-next-line no-unused-vars
        const rule = new adguard.rules.CssFilterRule(ruleText);
    }, `Css injection rule with 'url' was omitted: ${ruleText}`);
});

// https://github.com/AdguardTeam/AdguardBrowserExtension/issues/1444
QUnit.test('Inject rules with backslash should be omitted', (assert) => {
    let ruleText = 'example.com#$#body { background: \\75 rl(http://example.org/empty.gif) }';
    assert.throws(() => {
        // eslint-disable-next-line no-unused-vars
        const rule = new adguard.rules.CssFilterRule(ruleText);
    }, `Css injection rule with '\\' was omitted: ${ruleText}`);

    ruleText = 'example.org#$#body { background:u\\114\\0154("http://example.org/image.png"); }';
    assert.throws(() => {
        // eslint-disable-next-line no-unused-vars
        const rule = new adguard.rules.CssFilterRule(ruleText);
    }, `Css injection rule with '\\' was omitted: ${ruleText}`);

    let validRule = new adguard.rules.CssFilterRule('example.com#$#body { background: black; }');
    assert.ok(validRule !== null);

    validRule = new adguard.rules.CssFilterRule('example.org#$?#div:matches-css(width: /\d+px/) { background-color: red!important; }');
    assert.ok(validRule !== null);
});

QUnit.test('Css Filter Rule for wp.pl domain', (assert) => {
    const ruleText = 'www.example.org,~example.org##.test';
    const rule = new adguard.rules.CssFilterRule(ruleText);

    assert.ok(rule.getRestrictedDomains().length > 0, 'Restricted domain has been added');
    assert.equal(1, rule.getRestrictedDomains().length);
    assert.ok(rule.getPermittedDomains().indexOf('www.example.org') >= 0, 'Permitted domain has been added');
    assert.ok(rule.getRestrictedDomains().indexOf('example.org') >= 0, 'Restricted domain is equal to example.org');
    assert.equal('.test', rule.cssSelector);
});
