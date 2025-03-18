#!/usr/bin/env python3

import argparse
import json
import logging
from pydoc import locate

import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.requests_helpers as urequests

TRUE = ['true', '1', 't', 'y', 'yes']
FALSE = ['', 'false', '0', 'f', 'n', 'no']


def parse_args():
    parser = argparse.ArgumentParser(description="set query weights for query group")
    uargs.add_verbosity(parser)
    parser.add_argument("--query-group-id", help="query group id", required=True, type=int)
    parser.add_argument("--token", help="OAuth token", required=True)
    parser.add_argument("--author", help="Author", required=True)
    parser.add_argument("--params-tsv", help="Queries weights in TSV format", required=True)
    parser.add_argument(
        "--key-columns",
        help="TSV key columns description. Available columns: text, regionId, device, country",
        nargs="+",
        default=["text", "regionId", "device", "country"],
    )
    parser.add_argument(
        "--param-columns",
        help="""
            TSV value columns description,
            format: name(:default(:type)),
              defaults:
                type=str, if type==bool => default value should be in
                  {} or {}

            for example: weight:0 or robot:False:bool
            """.format(TRUE, FALSE),
        nargs="+",
        required=True
    )
    parser.add_argument("--replace-labels", default=False, action="store_true", help="replace old labels")
    parser.add_argument("--remove-old-params", default=False, action="store_true", help="remove old params")
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


def parse_tsv_values(params_tsv, key_columns, param_columns):
    query_values = {}
    new_labels = set()
    keys_count = len(key_columns)
    keys_params_count = len(key_columns) + len(param_columns)
    with ufile.fopen_read(params_tsv, use_unicode=True) as inp:
        # csv.reader doesn't support unicode
        for line in inp:
            if line:
                tokens = line.split("\t")
                assert len(tokens) >= keys_params_count, "columns count in input file < keys + params count"
                key = tuple(token for token, column in zip(tokens, key_columns))
                value = {}
                for token, column in zip(tokens[keys_count:], param_columns):
                    token = token.strip()
                    if column.is_label:
                        if column.name not in value:
                            value[column.name] = []
                        if not column.index:
                            column.index = len(value[column.name])
                        value[column.name].append(column.cast(token))
                        new_labels.add(column.cast(token))
                    else:
                        value[column.name] = column.cast(token)
                query_values[key] = value
    return query_values, new_labels


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


def clean_query_params(query_params, param_columns):
    return [query_param for query_param in query_params if query_param["name"] not in param_columns]


def set_query_params(queries, query_values, new_labels, tsv_columns, param_columns, replace_labels, remove_old_params):
    for query in queries:
        key = tuple(query[column] for column in tsv_columns)
        query_params = clean_query_params(query.get("params", []), param_columns) if not remove_old_params else []
        query_labels = query.get("labels", []) if not replace_labels else []
        query_labels = list(set(query_labels) - new_labels)
        for param in param_columns:
            if key in query_values:
                if param.is_label:
                    value = query_values[key][param.name][param.index]
                    if value:
                        query_labels.append(value)
                else:
                    query_params.append({"name": param.name, "value": query_values[key][param.name]})
            else:
                logging.debug("Value is not specified for query %s", query)
                if param.default:
                    if param.is_label:
                        query_labels.append(param.default)
                    else:
                        query_params.append({"name": param.name, "value": param.default})
        query["params"] = query_params
        query["labels"] = query_labels


class Param(object):
    def __init__(self, param):
        fields = param.split(':')
        self.name = fields[0]
        self.default = fields[1] if len(fields) > 1 else ''
        self.value_type = str
        if len(fields) > 2:
            self.value_type = locate(fields[2].lower())
            if self.value_type == bool:
                assert self.default in TRUE + FALSE
                self.default = True if self.default in TRUE else False
            else:
                self.default = self.value_type(self.default)
        self.is_label = True if self.name == 'labels' else False
        self.index = None

    def cast(self, value):
        if value:
            return self.value_type(value)
        else:
            return self.default

    def __eq__(self, name):
        return self.name == name


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    logging.info("Getting queries by query group id %s", cli_args.query_group_id)
    queries = get_queries(cli_args.query_group_id, cli_args.token)
    logging.info("Loaded %d queries", len(queries))

    param_columns = [Param(column) for column in cli_args.param_columns]

    logging.info("Parsing tsv")
    query_values, new_labels = parse_tsv_values(cli_args.params_tsv, cli_args.key_columns, param_columns)
    logging.info("Parsed %d queries", len(query_values))

    set_query_params(
        queries,
        query_values,
        new_labels,
        cli_args.key_columns,
        param_columns,
        cli_args.replace_labels,
        cli_args.remove_old_params
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
