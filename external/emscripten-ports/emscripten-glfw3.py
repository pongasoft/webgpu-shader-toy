# Copyright (c) 2024 pongasoft
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# @author Yan Pujante

import os
from typing import Union, Dict

TAG = '3.4.0.20240907'
HASH = '48aba179ef50b5efd564535cd549528d06f74d18f7028a06212fccc425b784106b0efb3aa7bf72da6fb80c32a31b410f13a87d4599417cad8dc3af15664fa1b2'
ZIP_URL = f'https://github.com/pongasoft/emscripten-glfw/releases/download/v{TAG}/emscripten-glfw3-{TAG}.zip'

# contrib port information (required)
URL = 'https://github.com/pongasoft/emscripten-glfw'
DESCRIPTION = 'This project is an emscripten port of GLFW 3.4 written in C++ for the web/webassembly platform'
LICENSE = 'Apache 2.0 license'

VALID_OPTION_VALUES = {
    'disableWarning': ['true', 'false'],
    'disableJoystick': ['true', 'false'],
    'disableMultiWindow': ['true', 'false'],
    'optimizationLevel': ['0', '1', '2', '3', 'g', 's', 'z']  # all -OX possibilities
}

OPTIONS = {
    'disableWarning': 'Boolean to disable warnings emitted by the library',
    'disableJoystick': 'Boolean to disable support for joystick entirely',
    'disableMultiWindow': 'Boolean to disable multi window support which makes the code smaller and faster',
    'optimizationLevel': f'Optimization level: {VALID_OPTION_VALUES["optimizationLevel"]} (default to 2)',
}

# user options (from --use-port)
opts: Dict[str, Union[str, bool]] = {
    'disableWarning': False,
    'disableJoystick': False,
    'disableMultiWindow': False,
    'optimizationLevel': '2'
}

port_name = 'emscripten-glfw3'


def get_lib_name(settings):
    return (f'lib_{port_name}_{TAG}-O{opts["optimizationLevel"]}' +
            ('-nw' if opts['disableWarning'] else '') +
            ('-nj' if opts['disableJoystick'] else '') +
            ('-sw' if opts['disableMultiWindow'] else '') +
            '.a')


def get(ports, settings, shared):
    # get the port
    ports.fetch_project(port_name, ZIP_URL, sha512hash=HASH)

    def create(final):
        root_path = os.path.join(ports.get_dir(), port_name)
        source_path = os.path.join(root_path, 'src', 'cpp')
        source_include_paths = [os.path.join(root_path, 'external'), os.path.join(root_path, 'include')]

        flags = [f'-O{opts["optimizationLevel"]}']

        if opts['disableWarning']:
            flags += ['-DEMSCRIPTEN_GLFW3_DISABLE_WARNING']

        if opts['disableJoystick']:
            flags += ['-DEMSCRIPTEN_GLFW3_DISABLE_JOYSTICK']

        if opts['disableMultiWindow']:
            flags += ['-DEMSCRIPTEN_GLFW3_DISABLE_MULTI_WINDOW_SUPPORT']

        ports.build_port(source_path, final, port_name, includes=source_include_paths, flags=flags)

    lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    if os.path.getmtime(lib) < os.path.getmtime(__file__):
        clear(ports, settings, shared)
        lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    return [lib]


def clear(ports, settings, shared):
    shared.cache.erase_lib(get_lib_name(settings))


def linker_setup(ports, settings):
    root_path = os.path.join(ports.get_dir(), port_name)
    source_js_path = os.path.join(root_path, 'src', 'js', 'lib_emscripten_glfw3.js')
    settings.JS_LIBRARIES += [source_js_path]


# Makes GLFW includes accessible without installing them (ex: #include <GLFW/glfw3.h>)
def process_args(ports):
    root_path = os.path.join(ports.get_dir(), port_name)
    args = [
        '-I', os.path.join(root_path, 'external'),  # <GLFW/glfw3.h>
        '-I', os.path.join(root_path, 'include'),   # <GLFW/emscripten_glfw3.h>
        f'-DEMSCRIPTEN_USE_PORT_CONTRIB_GLFW3={TAG.replace(".", "")}'
    ]
    return args


def check_option(option, value, error_handler):
    if value not in VALID_OPTION_VALUES[option]:
        error_handler(f'[{option}] can be {list(VALID_OPTION_VALUES[option])}, got [{value}]')
    if isinstance(opts[option], bool):
        value = value == 'true'
    return value


def handle_options(options, error_handler):
    for option, value in options.items():
        opts[option] = check_option(option, value.lower(), error_handler)
