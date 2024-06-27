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
from typing import Union, Dict, Optional
from tools import utils
import shutil

TAG = '1.90.5'

DISTRIBUTIONS = {
    'master': {
        'hash': 'b3e7e0dad187e853a3117ca73ba9a673589f3060486a1cd5ebc639d348e51cd6182190dd61ad72325eb3bdb84e107e02ff229ce6b311fc51899766f7a84aa1f7'
    },
    'docking': {
        'hash': '182d082201433e3092e48c8f8f097d3888ad32f9ef7d0c94e03b8337f1c334a590aaac7bdb2ab9c89a5dad896fa725bccf22d8c96e0bf6adbaa37795d6067103'
    }
}

# contrib port information (required)
URL = 'https://github.com/ocornut/imgui'
DESCRIPTION = f'Dear ImGui ({TAG}): Bloat-free Graphical User interface for C++ with minimal dependencies'
LICENSE = 'MIT License'

VALID_OPTION_VALUES = {
    'renderer': {'opengl3', 'wgpu'},
    'backend': {'sdl2', 'glfw'},
    'branch': DISTRIBUTIONS.keys()
}

# key is backend, value is set of possible renderers
VALID_RENDERERS = {
    'glfw': {'opengl3', 'wgpu'},
    'sdl2': {'opengl3'}
}

OPTIONS = {
    'renderer': f'Which renderer to use: {VALID_OPTION_VALUES["renderer"]} (required)',
    'backend': f'Which backend to use: {VALID_OPTION_VALUES["backend"]} (required)',
    'branch': 'Which branch to use: master (default) or docking',
    'disableDemo': 'A boolean to disable ImGui demo (enabled by default)',
    'disableImGuiStdLib': 'A boolean to disable misc/cpp/imgui_stdlib.cpp (enabled by default)',
}

# user options (from --use-port)
opts: Dict[str, Union[Optional[str], bool]] = {
    'renderer': None,
    'backend': None,
    'branch': 'master',
    'disableDemo': False,
    'disableImGuiStdLib': False
}

deps = []

build_deps = {}


def get_tag():
    return TAG if opts['branch'] == 'master' else f'{TAG}-{opts["branch"]}'


def get_zip_url():
    return f'https://github.com/ocornut/imgui/archive/refs/tags/v{get_tag()}.zip'


def get_lib_name(settings):
    return (f'lib_{name}_{get_tag()}-{opts["backend"]}-{opts["renderer"]}' +
            ('-nd' if opts['disableDemo'] else '') +
            '.a')


def get(ports, settings, shared):
    if opts['backend'] is None or opts['renderer'] is None:
        utils.exit_with_error(f'imgui port requires both backend and renderer options to be defined')

    ports.fetch_project(name, get_zip_url(), sha512hash=DISTRIBUTIONS[opts['branch']]['hash'])

    def create(final):
        root_path = os.path.join(ports.get_dir(), name, f'imgui-{get_tag()}')
        source_path = root_path

        # patching ImGui to use contrib.glfw3 (until ImGui is updated)
        patch_src_directory = os.path.join(os.path.dirname(os.path.abspath(__file__)))
        patch_dst_directory = os.path.join(root_path, 'backends')
        for patch_file in ['imgui_impl_glfw.cpp']:
            shutil.copy(os.path.join(patch_src_directory, patch_file), patch_dst_directory)

        # this port does not install the headers on purpose (see process_args)
        # a) there is no need (simply refer to the unzipped content)
        # b) avoids any potential issue between docking/master headers being different

        srcs = ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp']
        if not opts['disableDemo']:
            srcs.append('imgui_demo.cpp')
        if not opts['disableImGuiStdLib']:
            srcs.append('misc/cpp/imgui_stdlib.cpp')
        srcs.append(os.path.join('backends', f'imgui_impl_{opts["backend"]}.cpp'))
        srcs.append(os.path.join('backends', f'imgui_impl_{opts["renderer"]}.cpp'))

        flags = [f'--use-port={build_deps[dep]}' for dep in deps]
        flags.append('-DEMSCRIPTEN_USE_PORT_CONTRIB_GLFW3')

        ports.build_port(source_path, final, name, srcs=srcs, flags=flags)

    lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    if os.path.getmtime(lib) < os.path.getmtime(__file__):
        clear(ports, settings, shared)
        lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    return [lib]


def clear(ports, settings, shared):
    shared.cache.erase_lib(get_lib_name(settings))


def process_args(ports):
    # makes the imgui files accessible directly (ex: #include <imgui.h>)
    args = ['-I', os.path.join(ports.get_dir(), name, f'imgui-{get_tag()}')]
    if opts['branch'] == 'docking':
        args += ['-DIMGUI_ENABLE_DOCKING=1']
    if opts['disableDemo']:
        args += ['-DIMGUI_DISABLE_DEMO=1']
    return args


def linker_setup(ports, settings):
    if opts['renderer'] == 'wgpu':
        settings.USE_WEBGPU = 1


def check_option(option, value, error_handler):
    if value not in VALID_OPTION_VALUES[option]:
        error_handler(f'[{option}] can be {list(VALID_OPTION_VALUES[option])}, got [{value}]')
    return value


def check_required_option(option, value, error_handler):
    if opts[option] is not None and opts[option] != value:
        error_handler(f'[{option}] is already set with incompatible value [{opts[option]}]')
    return check_option(option, value, error_handler)


def handle_options(options, error_handler):
    for option, value in options.items():
        value = value.lower()
        if option == 'renderer' or option == 'backend':
            opts[option] = check_required_option(option, value, error_handler)
        elif option == 'branch':
            opts[option] = check_option(option, value, error_handler)
        elif option == 'disableDemo':
            if value in {'true', 'false'}:
                opts[option] = value == 'true'
            else:
                error_handler(f'{option} is expecting a boolean, got {value}')

    if opts['backend'] is None or opts['renderer'] is None:
        error_handler(f'both backend and renderer options must be defined')

    if opts['renderer'] not in VALID_RENDERERS[opts['backend']]:
        error_handler(f'backend [{opts["backend"]}] does not support [{opts["renderer"]}] renderer')

    if opts['backend'] == 'glfw':
        deps.append('emscripten-glfw3')
        patch_src_directory = os.path.join(os.path.dirname(os.path.abspath(__file__)))
        build_deps['emscripten-glfw3'] = os.path.join(patch_src_directory, 'emscripten-glfw3.py')
    else:
        deps.append('sdl2')
        build_deps['sdl2'] = 'sdl2'

