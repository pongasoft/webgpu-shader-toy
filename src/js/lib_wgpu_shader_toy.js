/*
 * Copyright (c) 2024 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */
let wgpu_shader_toy = {
  $WGPU_SHADER_TOY__deps: ['$stringToNewUTF8', 'free'],
  $WGPU_SHADER_TOY__postset: `
    // exports
    Module["wgpuShaderToyLoadFile"] = WGPU_SHADER_TOY.loadFile;
    `,
  $WGPU_SHADER_TOY: {
    fNewFileHandler: null, // {handler: <fn(userData, filename, content)>, userData: <ptr>}
    fBeforeUnloadHandler: null, // // {handler: <fn(userData)>, userData: <ptr>}

    // onNewFile
    onNewFile: (fragmentShaderFile, fragmentShaderContent) => {
      if(WGPU_SHADER_TOY.fNewFileHandler) {
        const filename = stringToNewUTF8(fragmentShaderFile.name);
        const content = stringToNewUTF8(fragmentShaderContent);
        {{{ makeDynCall('vppp', 'WGPU_SHADER_TOY.fNewFileHandler.handler') }}}(WGPU_SHADER_TOY.fNewFileHandler.userData, filename, content);
        _free(content);
        _free(filename);
      }
    },

    // loadFile
    loadFile: (file) => {
      let reader = new FileReader();
      reader.onload = function(evt) {
        WGPU_SHADER_TOY.onNewFile(file, evt.target.result);
      };
      reader.readAsText(file);
    },

    // createFileDialog
    createFileDialog: () => {
      WGPU_SHADER_TOY.fFragmentShaderFileDialog = document.createElement("input");
      WGPU_SHADER_TOY.fFragmentShaderFileDialog.type = 'file';
      WGPU_SHADER_TOY.fFragmentShaderFileDialog.onchange = (e) => {
        WGPU_SHADER_TOY.loadFile(e.target.files[0]);
        // implementation note: loading the same file twice does not work unless I recreate the input...
        WGPU_SHADER_TOY.createFileDialog();
      }
    }
  },

  // wgpu_shader_toy_install_handlers
  wgpu_shader_toy_install_handlers: (new_file_handler, before_unload_handler, user_data) => {
    WGPU_SHADER_TOY.fNewFileHandler = {
      handler: new_file_handler,
      userData: user_data
    };
    WGPU_SHADER_TOY.fBeforeUnloadHandler = {
      handler: before_unload_handler,
      userData: user_data
    };
    window.addEventListener('beforeunload', function (event) {
      if(!Module.resetRequested) {
        if(WGPU_SHADER_TOY.fBeforeUnloadHandler) {
          {{{ makeDynCall('vp', 'WGPU_SHADER_TOY.fBeforeUnloadHandler.handler') }}}(WGPU_SHADER_TOY.fBeforeUnloadHandler.userData);
        }
      }
    });
    WGPU_SHADER_TOY.createFileDialog();
  },

  // wgpu_shader_toy_uninstall_handlers
  wgpu_shader_toy_uninstall_handlers: () => {
    delete WGPU_SHADER_TOY.fFragmentShaderFileDialog;
    delete WGPU_SHADER_TOY.fBeforeUnloadHandler;
    delete WGPU_SHADER_TOY.fNewFileHandler;
  },

  // wgpu_shader_toy_open_file_dialog
  wgpu_shader_toy_open_file_dialog: () => {
    console.log('wgpu_shader_toy_open_file_dialog()');
    if(WGPU_SHADER_TOY.fFragmentShaderFileDialog) {
      console.log('wgpu_shader_toy_open_file_dialog.click()');
      WGPU_SHADER_TOY.fFragmentShaderFileDialog.click();
    }
  },

  // wgpu_shader_toy_print_stack_trace
  wgpu_shader_toy_print_stack_trace: (message) => {
    message = message ? UTF8ToString(message) : null;
    const error = new Error(message);
    console.log(error.stack);
  },

  // wgpu_shader_toy_abort
  wgpu_shader_toy_abort: (message) => {
    message = message ? UTF8ToString(message) : null;
    const error = new Error(message);
    console.log(error.stack);
    if(Module.onAbort) {
      Module.onAbort(message);
    }
  },

  // wgpu_shader_toy_export_content
  wgpu_shader_toy_export_content: (filename, content) => {
    filename = filename ? UTF8ToString(filename): null;
    content = content ? UTF8ToString(content): null;

    const blob = new Blob([content], { type: "text/plain;charset=utf-8" });

    const downloadLink = document.createElement("a");
    downloadLink.href = URL.createObjectURL(blob);
    downloadLink.download = filename;

    document.body.appendChild(downloadLink);
    downloadLink.click();
    document.body.removeChild(downloadLink);
  },

  // wgpu_shader_toy_save_screenshot
  wgpu_shader_toy_save_screenshot: (glfwWindow, filename, type, quality) => {
    filename = filename ? UTF8ToString(filename): null;
    type = type ? UTF8ToString(type) : 'image/png';
    const canvas = Module.glfwGetCanvas(glfwWindow);
    if(canvas) {
      canvas.toBlob((blob) => {
        let url = URL.createObjectURL(blob);
        let a = document.createElement('a');
        document.body.append(a);
        a.download = filename;
        a.href = url;
        a.click();
        a.remove();
      }, type, quality);
    }
  },

}

autoAddDeps(wgpu_shader_toy, '$WGPU_SHADER_TOY')
mergeInto(LibraryManager.library, wgpu_shader_toy);
