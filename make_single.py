import argparse
import os
import datetime
import re


parser = argparse.ArgumentParser(
    description = 'Converts firebird library to a single header file')
parser.add_argument('--output', '-o', nargs = 1,
    help = 'path to output file', metavar = 'file', default = 'single/firebird.hpp')
args = parser.parse_args()

single_file = args.output if type(args.output) == str else args.output[0]
license_file = 'LICENSE.MIT'

includes = []
depend_include = re.compile(r'#include ["<](.*?)[>"]')
pragma_once_hpp = re.compile(r'#pragma once')

src_path = 'include'
files_to_process = [
    'firebird.hpp',
]


def is_include_guard(line):
    return pragma_once_hpp.match(line)


def get_project_include(line):
    depend_match = depend_include.match(line)
    if depend_match:
        inc_file = os.path.join(src_path, depend_match.group(1))
        if os.path.isfile(inc_file):
            return inc_file
    return None


def get_depend_include(line):
    depend_match = depend_include.match(line)
    if depend_match:
        return depend_match.group(1)
    return None


def build_intro():
    intro = """// {license}
// This file was generated with a script.
// Generated {time} UTC
#pragma once

"""
    if license_file:
        with open(license_file, 'r') as f:
            license = '// '.join(f.readlines())
    else:
        license = ''

    return intro.format(
        license = license,
        time = datetime.datetime.now(datetime.timezone.utc))


def process_file(filename, out):
    if filename in includes:
        return  # Already processed
    includes.append(filename)

    print('processing {}'.format(filename))

    out.write('// beginning of {}\n\n'.format(filename))
    empty_line_state = True

    with open(filename, 'r') as f:
        for line in f:
            # skip comments with indent 0, e.g. license
            if line.startswith('//'):
                continue

            # skip include guard non-sense
            if is_include_guard(line):
                continue

            name = get_project_include(line)
            if name:
                process_file(name, out)
                continue

            dependency = get_depend_include(line)
            if dependency:
                depend_file = os.path.join('dependency', dependency)
                if depend_file in includes:
                    continue
                includes.append(depend_file)

            empty_line = len(line.strip()) == 0
            if empty_line and empty_line_state:
                continue
            empty_line_state = empty_line

            # Source code line
            out.write(line)

    out.write('// end of {}\n\n'.format(filename))


if not os.path.exists('single'):
    os.makedirs('single')

with open(single_file, 'w') as f:
    f.write(build_intro())
    for src in files_to_process:
        process_file(os.path.join(src_path, src), f)

