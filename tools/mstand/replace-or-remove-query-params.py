import argparse
import json
import logging

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.requests_helpers as urequests


def parse_args():
    parser = argparse.ArgumentParser(description="set query weights for query group")
    uargs.add_verbosity(parser)
    parser.add_argument("--query-group-id", help="query group id", required=True, type=int)
    parser.add_argument("--token", help="OAuth token", required=True)
    parser.add_argument("--author", help="Author", required=True)
    parser.add_argument(
        "--remove-labels",
        help="list of label names to remove",
        nargs="+",
        default=[],
    )
    parser.add_argument(
        "--remove-params",
        help="list of param names to remove",
        nargs="+",
        default=[],
    )
    parser.add_argument(
        "--replace-old-label-name",
        help="new name of label or param",
        default=None,
    )
    parser.add_argument(
        "--replace-new-label-name",
        help="new name of label or param",
        default=None,
    )
    parser.add_argument(
        "--replace-old-param-name",
        help="new name of label or param",
        default=None,
    )
    parser.add_argument(
        "--replace-new-param-name",
        help="new name of label or param",
        default=None,
    )
    parser.add_argument("--update-request-timeout", default=600, type=int, help="query groups update request timeout")

    return parser.parse_args()


def headers(token):
    return {
        "Authorization": "OAuth {}".format(token),
        "Content-Type": "application/json;charset=UTF-8",
    }


def get_queries(query_group_id, token):
    url = "https://metrics-qgaas.metrics.yandex-team.ru/api/basket/{}/query".format(query_group_id)
    response = urequests.retry_request(method="GET", url=url, verify=False, headers=headers(token))
    return json.loads(response.text)


def put_queries(queries, query_group_id, author, token, timeout):
    # TODO: replace with urllib.urlencode
    url = "https://metrics-qgaas.metrics.yandex-team.ru/api/basket/{}/query-generator?author={}&comment=".format(
        query_group_id,
        author
    )
    data = {"type": "RAW", "queries": queries}
    response = urequests.retry_request(
        method="PUT",
        url=url,
        json=data,
        verify=False,
        timeout=timeout,
        headers=headers(token),
    )
    if response.status_code < 200 or response.status_code >= 300:
        raise Exception("Error with code {}: {}".format(response.status_code, response.text))


def clean_query_params(query_params, remove_params):
    params = []
    for query_param in query_params:
        if query_param["name"] not in remove_params:
            params.append(query_param)
    return params


def clean_query_labels(query_labels, remove_labels):
    labels = []
    for query_label in query_labels:
        if query_label not in remove_labels:
            labels.append(query_label)
    return labels


def set_query_params(
        queries,
        remove_labels,
        remove_params,
        replace_old_label_name,
        replace_new_label_name,
        replace_old_param_name,
        replace_new_param_name
):
    for query in queries:
        query_params = clean_query_params(query.get("params", []), remove_params)
        query_labels = clean_query_labels(query.get("labels", []), remove_labels)
        if replace_old_param_name and replace_new_param_name:
            for query_param in query_params:
                if query_param["name"] == replace_old_param_name:
                    query_param["name"] = replace_new_param_name
        if replace_old_label_name and replace_new_label_name:
            if replace_old_label_name in query_labels:
                query_labels.remove(replace_old_label_name)
                query_labels.append(replace_new_label_name)
        query["params"] = query_params
        query["labels"] = query_labels


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    logging.info("Getting queries by query group id %s", cli_args.query_group_id)
    queries = get_queries(cli_args.query_group_id, cli_args.token)
    logging.info("Loaded %d queries", len(queries))

    set_query_params(
        queries,
        cli_args.remove_labels,
        cli_args.remove_params,
        cli_args.replace_old_label_name,
        cli_args.replace_new_label_name,
        cli_args.replace_old_param_name,
        cli_args.replace_new_param_name
    )
    put_queries(
        queries,
        cli_args.query_group_id,
        cli_args.author,
        cli_args.token,
        cli_args.update_request_timeout,
    )


if __name__ == "__main__":
    main()
