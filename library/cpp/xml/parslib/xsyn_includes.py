#! /usr/bin/env python


import os


def get_include_callback(xsyn_dir):
    """
    .. function: get_include_callback    returns function that processes each DOM element to get xsyn include from it, and it's aware of directory with all the xsyns.

        :param  xsyn_dir    directory with xsyns.
    """
    def get_include(element):
        """
        .. function:    get_include     returns list of includes from this DOM element.

            :param  element     DOM element.
        """
        res = []
        if element.nodeType == element.ELEMENT_NODE and element.nodeName == "parse:include":
            attrs = element.attributes
            for i in xrange(attrs.length):
                attr = attrs.item(i)
                if attr.nodeName == "path":
                    include_filename = attr.nodeValue
                    res.append(include_filename)

                    include_includes = process_xsyn(os.path.join(xsyn_dir, include_filename), get_include)

                    res += include_includes
        return res

    return get_include



def traverse_xsyn(element, on_element):
    """
    .. function: traverse_xsyn  traverses element and returns concatenated lists of calling on_element of each element.

        :param  element     element in DOM.
        :param  on_element  callback on element that returns list of values.
    """
    res = on_element(element)
    for child in element.childNodes:
        child_results = traverse_xsyn(child, on_element)
        res += child_results
    return res



def process_xsyn(filepath, on_element):
    """
    .. function:    process_xsyn    processes xsyn file and return concatenated list of calling on_element on each DOM element.

        :param  filepath    path to xsyn file
        :param  on_element  callback called on each element in xsyn that returns list of values.

    .. note: this is not paralellizable function because of file_stack attribute.
    """

    # keep a stack of filepathes if on_element calls process_xsyn recursively
    if "files_stack" not in process_xsyn.__dict__:
        process_xsyn.files_stack = []
    if filepath in process_xsyn.files_stack:
        begin = process_xsyn.files_stack.index(filepath)
        files_chain = process_xsyn.files_stack[begin:] + [filepath]
        raise RuntimeError("found recusing includes:\n{0}".format("\n".join([" " * n + filename for n, filename in enumerate(files_chain)])))
    process_xsyn.files_stack.append(filepath)

    with open(filepath) as xsyn_file:
        from xml.dom.minidom import parse, Element
        tree = parse(xsyn_file)
        tree.normalize()
        res = traverse_xsyn(tree, on_element)

    process_xsyn.files_stack.pop()
    return res



def main(argv):
    from argparse import ArgumentParser
    parser = ArgumentParser(description = "Get all xsyn files that are used to build the specified xsyn file.")

    parser.add_argument("-i", "--input", required = True, help = "Input xsyn file")

    parsed_args = parser.parse_args(argv[1:])

    _, ext = os.path.splitext(parsed_args.input)
    if ext != ".xsyn":
        raise RuntimeError("file {0} doesn't have extension xsyn and does not believed to be xsyn file".format(parsed_args.input))

    callback = get_include_callback(os.path.dirname(parsed_args.input))
    includes = process_xsyn(parsed_args.input, callback)
    unique_includes = [val.strip() for val in sorted(set(includes))]
    for include in unique_includes:
        print include



if __name__ == "__main__":
    from sys import argv
    main(argv)
