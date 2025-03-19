import json
import os
import argparse
from string import Template

html_template = Template(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Active jobs</title>
</head>
<body>
<font size="4">
<table align="center" style="width:100%, height:100%" cellpadding="8" border="1">
$table
</table>
</body>
</html>
""")

row_template = Template("<tr><th>$instance</th><th>$host</th><th>$owner</th><th>$created</th><th>$url</th></tr>\n")

parser = argparse.ArgumentParser()
parser.add_argument('--cli', dest='use_cli', action='store_true')
parser.add_argument('--browser', dest='use_cli', action='store_false')
parser.add_argument('--processor_ip', type=str)
parser.add_argument('--token', type=str)
parser.set_defaults(use_cli=True)
args = parser.parse_args()

stream = os.popen(f'ssh -T {args.processor_ip} \'curl -s -H "Authorization: OAuth {args.token}" http://[{args.processor_ip}]:6000/api/cloud-agent/event/active-jobs\'')
output = stream.read()
result = json.loads(output)

if args.use_cli:
    for item in result:
        print(f'Job on {item["host"]}, created at {item["createdAt"]} by {item["owner"]}, instance id: {item["instanceId"]}')
        print(f'Job URL: {item["workflowURL"]}\n')
else:
    table_content = ""
    table_content += row_template.substitute(instance="Instance", host="Host", owner="Owner", created="Created at",
                                             url="Workflow URL")
    for item in result:
        table_content += row_template.substitute(instance=item["instanceId"], host=item["host"], owner=item["owner"],
                                                 created=item["createdAt"],
                                                 url=f'<a href="{item["workflowURL"]}">{item["workflowURL"]}</a>')

    html = html_template.substitute(table=table_content)
    with open("active_jobs.html", "w") as out_file:
        out_file.write(html)

    os.system("open active_jobs.html")
