'''
@author: ashishkin
'''

header = """<html>
<head>
    <title>Snippets compare. Hide equal $$eq$$, Randomize $$rnd$$</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8">

<script type="text/javascript"><!--

function getInverseId(id) {
    prefix = id.substring(0, 2)
    number = id.substring(3)
    var inverse = ""
    if (prefix == "s1") {
        inverse = "s2-" + number
    } else {
        inverse = "s1-" + number
    }
    return inverse
}

function calculate() 
{ 
    var total = $$total$$
    
    leftSum = 0
    rightSum = 0
    middleSum = 0
    
    var leftChecked = []
    var rightChecked = []
    var middleChecked = []
    
    for (var i = 0; i <= total; i++) {
        var left = document.getElementById("s1-" + i)
        if (left != null && left.checked) {
            leftSum++
            leftChecked.push("s1-" + i)
        }
        
        var right = document.getElementById("s2-" + i)
        if (right != null && right.checked) {
            rightSum++
            rightChecked.push("s2-" + i)
        }
        
        var middle = document.getElementById("middle-" + i)
        if (middle != null && middle.checked) {
            middleSum++
            middleChecked.push("middle-" + i)
        }
    }
    
    var res = document.getElementById("stats")
    for (i = 0; i < res.childNodes.length; i++) 
    {
        res.removeChild(res.childNodes[i]);
    }
    
    res.appendChild(document.createTextNode("Left: " + leftSum + " Right: " + rightSum + " Equal: " + middleSum))
    
    var dump = document.getElementById("dump")
    for (i = 0; i < dump.childNodes.length; i++) 
    {
        dump.removeChild(dump.childNodes[i]);
    }
    var dumpTxt = 'Left\\n' + leftChecked.join('\\n') + '\\n\\nMiddle\\n' + middleChecked.join('\\n') + '\\n\\nRight\\n' + rightChecked.join('\\n')
    dump.appendChild(document.createTextNode(dumpTxt))
    
}
--></script>

<style type="text/css"><!--
DIV.h { color: blue; }
DIV.t {}
DIV.m {color: red; }
DIV.u { color: green; }
DIV.q { color: #a0a0a0; text-align: center; }
TD.m { background-color: #e0e0e0; }
TD.q { }
--></style>
    
</head>
<body>
<table border="1" width="90%">
"""

queryRow = """<tr>
    <td colspan="3" class="q">
        <div class="q">$$q$$</div>
    </td>
</tr>
"""

snipRow = """<tr>
    <td class="s">
        <div class="h">$$snipTitle1$$</div>
        <div class="m">$$snipHeadline1$$</div>
        <div class="t">$$snipTxt1$$</div>
        <div class="u">$$snipUrl1$$</div>
    </td>
    <td class="m" width="1%">
        <nobr>
            <fieldset>
                <input name="$$num$$" id="$$left$$-$$num$$" type="radio" value="$$left$$" />
                <input name="$$num$$" id="middle-$$num$$" type="radio" value="middle"/>
                <input name="$$num$$" id="$$right$$-$$num$$" type="radio" value="$$right$$""/>
            </fieldset>
        </nobr>
    </td>
    <td class="s">
        <div class="h">$$snipTitle2$$</div>
        <div class="m">$$snipHeadline2$$</div>
        <div class="t">$$snipTxt2$$</div>
        <div class="u">$$snipUrl2$$</div>
    </td>
</tr>
"""

footer = """</table>
<div>
<input type="button" value="Calculate" onclick="calculate();"/>
<div id="stats">
Stats.
</div>
<pre id="dump"/>
</div>

</body>
"""
