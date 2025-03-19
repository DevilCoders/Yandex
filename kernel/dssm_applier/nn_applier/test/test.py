#!/usr/bin/env python

import os
import sys
import os.path
import yatest.common


def lines_equal(line1, line2):
    items1 = [float(item) for item in line1.split()]
    items2 = [float(item) for item in line2.split()]
    if len(items1) != len(items2):
        return False
    for v1, v2 in zip(items1, items2):
        if abs(v1 - v2) / (abs(v1) + abs(v2)) > 0.001 and abs(v1 - v2) > 0.001:
            return False
        return True


datasets_dir = yatest.common.data_path("dssm/inputs")
outputs_dir = yatest.common.data_path("dssm/outputs")
default_header = "target,weight,query,region,doc_snippet,doc_title,doc_url,doc_uta,doc_uta_url"
default_input = os.path.join(datasets_dir, "input1")
applier_binary = yatest.common.binary_path('kernel/dssm_applier/nn_applier/nn_applier')
temp_model_name = "tmp_model.appl"


def do_test_model(name, header, variable, input_path, output_path):
    with open(input_path) as input_f, open("out", "wb") as out_f:
        yatest.common.execute(
            [applier_binary, 'apply', '-m', name, "--header", header, '-o', variable],
            stdin=input_f, stdout=out_f, check_exit_code=True, shell=True,
        )
    with open("out") as f:
        output = [line.strip("\n") for line in f]
    with open(output_path) as f:
        true_output = [line.strip("\n") for line in f]

    assert len(output) == len(true_output)
    for idx, (line_new, line_old) in enumerate(zip(output, true_output)):
        if not lines_equal(line_new, line_old):
            sys.stderr.write("bad model : " + name + "\n")
            sys.stderr.write("line " + str(idx) + ":" + "\n")
            sys.stderr.write(line_new.strip() + "\n")
            sys.stderr.write(line_old.strip() + "\n")
            assert False


def test_search_models_uta():
    do_test_model(
        name="search_models.appl",
        header=default_header,
        variable="joint_output_uta_softsign",
        input_path=default_input,
        output_path=os.path.join(outputs_dir, "search_uta_output"))


def test_search_models_bigrams_logdwelltime():
    do_test_model(
        name="search_models.appl",
        header=default_header,
        variable="joint_output_bigrams_softsign",
        input_path=default_input,
        output_path=os.path.join(outputs_dir, "search_bigrams_output"))


def test_search_models_ctr():
    do_test_model(
        name="search_models.appl",
        header=default_header,
        variable="joint_output_ctr_softsign",
        input_path=default_input,
        output_path=os.path.join(outputs_dir, "search_ctr_output"))


def test_search_models_multiclick():
    do_test_model(
        name="search_models.appl",
        header=default_header,
        variable="joint_output_multiclick",
        input_path=default_input,
        output_path=os.path.join(outputs_dir, "search_multiclick_output"))


def test_search_models_logdwelltime():
    do_test_model(
        name="search_models.appl",
        header=default_header,
        variable="joint_output_log_dwelltime",
        input_path=default_input,
        output_path=os.path.join(outputs_dir, "search_logdwelltime_output"))


def test_ctrserp_click_vs_noclickeasy_normquant256bin_128bit():
    do_test_model(
        name="ctrserp_click_vs_noclickeasy_normquant256bin_128bit.compressed.apply",
        header="target,weight,query,region,snippet,title,stripped_goodurl,goodurl_uta,uta2,domain",
        variable="joint_output",
        input_path=default_input,
        output_path=os.path.join(
            outputs_dir,
            "ctrserp_click_vs_noclickeasy_normquant256bin_128bit.compressed.apply"))


def test_ctrserp_click_vs_noclickhard_em300_normquant16bin_128bit_annealed_compressed():
    do_test_model(
        name="ctrserp_click_vs_noclickhard_em300_normquant16bin_128bit_annealed.compressed5.apply",
        header="target,weight,query,region,snippet,title,stripped_goodurl,goodurl_uta,uta2,domain",
        variable="joint_output",
        input_path=default_input,
        output_path=os.path.join(
            outputs_dir,
            "ctrserp_click_vs_noclickhard_em300_normquant16bin_128bit_annealed.compressed5.apply"))


def test_search_models_bigrams_logdwelltime_grads():
    do_test_model(
        name="search_models.appl",
        header=default_header,
        variable="grad_stats_softsign",
        input_path=default_input,
        output_path=os.path.join(outputs_dir, "search_bigrams_grads_output"))


def test_rsya_ctr_projected50():
    do_test_model(
        name="rsya_ctr50.appl",
        header="queries,region,topdomains,adhoc,bmcats,visitgoals,purchase_offers,cart_offers,detail_offers,"
               "purchase_counterids,cart_counterids,detail_counterids,ages,revenues,genders,detailed_device_types,"
               "browsers,title,snippet,stripped_goodurl,goodurl_uta,target_domain,landpage_title,banner_id,order_id",
        variable="joint_afterdot",
        input_path=os.path.join(datasets_dir, "input_rsya_ctr50"),
        output_path=os.path.join(outputs_dir, "output_rsya_ctr50")
    )


def test_rsya_dssmtsar2_ctr_bc_50_bc():
    do_test_model(
        name="jruziev_dssmtsar2_ctr_bc_50_2l.appl",
        header="queries,region,topdomains,adhoc,bmcats,visitgoals,purchase_offers,cart_offers,detail_offers,"
               "purchase_counterids,cart_counterids,detail_counterids,ages,revenues,genders,detailed_device_types,"
               "browsers,title,snippet,stripped_goodurl,goodurl_uta,target_domain,landpage_title,banner_id,order_id",
        variable="joint_afterdot#proj_bc",
        input_path=os.path.join(datasets_dir, "input_rsya_ctr50"),
        output_path=os.path.join(outputs_dir, "output_rsya_dssmtsar2_ctr_bc_50")
    )


def test_rsya_dssmtsar2_ctr_bc_50_ctr():
    do_test_model(
        name="jruziev_dssmtsar2_ctr_bc_50_2l.appl",
        header="queries,region,topdomains,adhoc,bmcats,visitgoals,purchase_offers,cart_offers,detail_offers,"
               "purchase_counterids,cart_counterids,detail_counterids,ages,revenues,genders,detailed_device_types,"
               "browsers,title,snippet,stripped_goodurl,goodurl_uta,target_domain,landpage_title,banner_id,order_id",
        variable="joint_afterdot#proj_ctr",
        input_path=os.path.join(datasets_dir, "input_rsya_ctr50"),
        output_path=os.path.join(outputs_dir, "output_rsya_ctr50")
    )


def test_jruziev_tsar_searchBC1():
    do_test_model(
        name="jruziev_tsar_searchBC1.appl",
        header="adhoc,ages,bmcats,browsers,cart_counterids,cart_offers,detail_counterids,detail_offers"
               ",detailed_device_types,genders,purchase_counterids,purchase_offers,queries,region,revenues"
               ",search_query,topdomains,visitgoals,banner_id,goodurl_uta,landpage_title,order_id,snippet"
               ",stripped_goodurl,target_domain,title",
        variable="joint_afterdot#proj_bc",
        input_path=os.path.join(datasets_dir, "input_sqynet"),
        output_path=os.path.join(outputs_dir, "output_jruziev_tsar_searchBC1")
    )


def test_models_optimization():
    yatest.common.execute(
        [applier_binary, "perform_optimizations", "-m", "search_models.appl", "-d", temp_model_name],
        check_exit_code=True, shell=True
    )

    variables = [
        "joint_output_uta_softsign",
        "joint_output_bigrams_softsign",
        "joint_output_ctr_softsign",
        "joint_output_multiclick",
        "joint_output_log_dwelltime"
    ]

    outputs = [
        "search_uta_output",
        "search_bigrams_output",
        "search_ctr_output",
        "search_multiclick_output",
        "search_logdwelltime_output"
    ]

    for var, out in zip(variables, outputs):
        do_test_model(
            name=temp_model_name,
            header=default_header,
            variable=var,
            input_path=default_input,
            output_path=os.path.join(outputs_dir, out)
        )
