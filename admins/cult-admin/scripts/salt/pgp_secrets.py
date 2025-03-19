#!/usr/bin/env python
''' Create/recrypt sls'es with gpg encrypted ssl
certificates for templates.certificates formula '''
''' author: aarseniev@yandex-team.ru '''

# TODO
# add debug key and debug logging for it
import sys
import gpg
import yaml
import argparse
import copy

def make_gpg_work(data, keys, action):
    ''' Takes dict in {'<file_name>': '<text>'} format, fingerprints and action
    Return dict in templates.certificates pillar format'''
    for f in data['pgp_secrets']:
        for k in f:
            # .replace - some kind of dark magic. Sometimes str(decrypt) returns \r\n not \n and pyyaml breaks :(
            f[k]['data'] = str( gpg.crypt( f[k]['data'], keys, action)).replace('\r\n', '\n')
    return data

def get_data(src_files, base_path):
    dic = {'pgp_secrets': []}
    for src_file in src_files:
        tmp_dic = {}
        tmp_dic[base_path + src_file.split('/')[-1]] = {'data': gpg.get_data(src_file)}
        dic['pgp_secrets'].append(tmp_dic)
    return dic

def get_sls(src_files):
    dic = {'pgp_secrets': []}
    for src_file in src_files:
        tmp_dic = {}
        with open(src_file,'r') as f:
            data = yaml.load(f)
        for f in data['pgp_secrets']:
            for k in f:
                tmp_dic[k] = f[k]
        dic['pgp_secrets'].append(tmp_dic)
    return dic

def make_sls(data):
    result = yaml.dump( data, default_flow_style=False, default_style='|')
    return result

def main():
    parser = argparse.ArgumentParser(add_help=True, description="Utility for create sls with crypted data and recrypt them")
    parser.add_argument('-a', dest='action', choices=['encrypt', 'decrypt', 'recrypt'], required=True, help='What to do')
    parser.add_argument('-o', dest='output', action='store', help='Output to file. If no: send to stdout')
    parser.add_argument('--file', action='append', required=True, help='Source file(s) with data to be encrypted/decrypted. Source file to recrypt')
    parser.add_argument('--path', action='append', required=True, help='Path to dir with *.gpg keys')
    parser.add_argument('--base-path', action='store', type=str, default='/etc/nginx/ssl/', help='Path for dest file in result sls. By default: /etc/nginx/ssl/')
    args = parser.parse_args()

    if len(sys.argv)==1:
        parser.print_help()
        sys.exit(0)

    fingers = gpg.load_keys(args.path)
    if args.action == 'encrypt':
        data = get_data(args.file, args.base_path)
    elif args.action == 'decrypt':
        data = get_sls(args.file)
    elif args.action == 'recrypt':
        data = get_sls(args.file)
        data = make_gpg_work(data, fingers, 'decrypt')
        args.action = 'encrypt'
    #    args.output = args.file[0]

    result = make_sls(make_gpg_work(data, fingers, args.action))
    if args.output:
        with open(args.output,'w') as f:
            f.write( result)
    else:
        print result

if __name__ == '__main__':
    main()
