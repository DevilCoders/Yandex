# Questions and answers about {{ yandex-cloud }} technical support

#### How do I contact technical support? {#support-channels}

You can contact technical support {% if product == "yandex-cloud" %}in the management console under [Support]({{ link-console-support }}){% endif %}{% if product == "cloud-il" %}by email at [{{ link-support-mail }}](mailto:{{ link-support-mail }}){% endif %}.

{% if product == "yandex-cloud" %}

#### How do I contact technical support if I can't log in to the management console? {#requesting-support-without-ui}

If you are unable to access your Yandex account, please see our [troubleshooting instructions]{% if lang == "ru" %}(https://yandex.ru/support/passport/troubleshooting/problems.html){% endif %}{% if lang == "en" %}(https://yandex.com/support/passport/troubleshooting/problems.html){% endif %} in the Yandex ID Help.

If you successfully logged in to your Yandex account but are unable to connect to the {{ yandex-cloud }} management console, contact technical support by email [{{ link-support-mail }}](mailto:{{ link-support-mail }}).

{% endif %}

#### What kinds of issues and cases does technical support resolve? {#support-cases-types}

{% if product == "yandex-cloud" %}

Depending on your service plan, you can contact support for various issues. For more information about service plans, see [{#T}](overview.md).

Regardless of your service plan, you can request the following from technical support:

* [Service logs](request.md#logs) related to your resources and actions in {{ yandex-cloud }}.
* [Your personal data](request.md#personal) stored by Yandex.

{% endif %}

{% if product == "cloud-il" %}

For a full list of issues that you can contact support with, see [{#T}](overview.md).

{% note info %}

The support team doesn't respond to user requests for the development of use cases and application manifests.

{% endnote %}

{% endif %}

#### How quickly does technical support respond? {#reaction-time}

{% if product == "yandex-cloud" %}

Technical support responds to requests 24 hours a day, 7 days a week.

The response time depends on your service plan. For more information about service plans, see [{#T}](overview.md).

{% endif %}

{% if product == "cloud-il" %}

{% include [arrangements](../_includes/support/arrangements.md) %}

{% endif %}

#### How quickly can technical support solve an identified problem? {#resolution-time}

The resolution time is individual for each case and depends on the nature of the problem. Problems with apps and services can have various causes. This makes it difficult to estimate how long it will take. The technical support team works closely with you to identify the cause of the problem and fix it as quickly as possible.

#### How many times can I contact technical support? {#support-requests-limit}

There is no limit to the number of times you can contact technical support.

#### How can I get help with architecture-related tasks? {#help-with-arch-tasks}

To get help with architecture-related tasks, use the feedback form on the management console support page.

#### What third-party software is covered by {{ yandex-cloud }} technical support? {#supported-third-party-sw}

Technical support includes troubleshooting third-party software that is used in conjunction with the cloud infrastructure and limited assistance in solving identified problems. Technical support engineers can help you with the installation, basic setup, and diagnostics of the following software:

- Operating systems on {{ yandex-cloud }} virtual machines created from public images provided by {{ yandex-cloud }}.
- Databases created with {% if product == "yandex-cloud" %}[{{ mpg-full-name }}](../managed-postgresql/), [{{ mch-full-name }}](../managed-clickhouse/), [{{ mmg-full-name }}](../managed-mongodb/){% endif %}{% if product == "cloud-il" %}[{{ mpg-full-name }}](../managed-postgresql/), and [{{ mch-full-name }}](../managed-clickhouse/){% endif %}.

#### What happens if you can't solve my third-party software problems? {#unresolved-third-party-issues}

If the technical support team can't resolve your third-party software issue, you need to contact the support service of the software vendor. In some cases, contacting a vendor's support service requires a valid contract for technical support with the vendor or their partners.
