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
    Module["wgpuShaderToyLoadFile"] = WGPU_SHADER_TOY.onFile;
    `,
  $WGPU_SHADER_TOY: {
    fUserData: null,
    fNewContentHandler: null, // <fn(userData, token, name, content, error)>
    fBeforeUnloadHandler: null, // <fn(userData)>
    fOnFileHandler: null, // <fn(userData, file)>

    // onNewContent
    onNewContent: (iToken, iName, iContent, iError) => {
      if(WGPU_SHADER_TOY.fNewContentHandler) {
        const name = stringToNewUTF8(iName);
        const content = iContent ? stringToNewUTF8(iContent) : null;
        const error = iError ? stringToNewUTF8(iError) : null;
        {{{ makeDynCall('vpippp', 'WGPU_SHADER_TOY.fNewContentHandler') }}}(WGPU_SHADER_TOY.fUserData, iToken, name, content, error);
        if(error)
          _free(error);
        if(content)
          _free(content);
        _free(name);
      }
    },

    // onFile
    onFile: (iFile) => {
      if(!iFile)
        return;
      if(WGPU_SHADER_TOY.fOnFileHandler) {
        const filename = stringToNewUTF8(iFile.name);
        const token = {{{ makeDynCall('ipp', 'WGPU_SHADER_TOY.fOnFileHandler') }}}(WGPU_SHADER_TOY.fUserData, filename);
        _free(filename);
        WGPU_SHADER_TOY.loadFile(token, iFile);
      }
    },

    // loadFile
    loadFile: (iToken, file) => {
      let reader = new FileReader();
      reader.onload = function(evt) {
        WGPU_SHADER_TOY.onNewContent(iToken, file.name, evt.target.result, null);
      };
      reader.onerror = function(evt) {
        WGPU_SHADER_TOY.onNewContent(iToken, file.name, null, reader.error.message);
      };
      reader.readAsText(file);
    },
  },

  // wgpu_shader_toy_install_handlers
  wgpu_shader_toy_install_handlers: (user_data, new_content_handler, before_unload_handler, on_file_handler) => {
    WGPU_SHADER_TOY.fUserData = user_data;
    WGPU_SHADER_TOY.fNewContentHandler = new_content_handler;
    WGPU_SHADER_TOY.fBeforeUnloadHandler = before_unload_handler;
    WGPU_SHADER_TOY.fOnFileHandler = on_file_handler;
    window.addEventListener('beforeunload', function (event) {
      if(!Module.resetRequested) {
        if(WGPU_SHADER_TOY.fBeforeUnloadHandler) {
          {{{ makeDynCall('vp', 'WGPU_SHADER_TOY.fBeforeUnloadHandler') }}}(WGPU_SHADER_TOY.fUserData);
        }
      }
    });
  },

  // wgpu_shader_toy_uninstall_handlers
  wgpu_shader_toy_uninstall_handlers: () => {
    delete WGPU_SHADER_TOY.fOnFileHandler;
    delete WGPU_SHADER_TOY.fBeforeUnloadHandler;
    delete WGPU_SHADER_TOY.fNewContentHandler;
    delete WGPU_SHADER_TOY.fUserData;
  },

  // wgpu_shader_toy_open_file_dialog
  wgpu_shader_toy_open_file_dialog: () => {
    const dialog = document.createElement("input");
    dialog.type = 'file';
    dialog.onchange = (e) => {
      if(e.target.files.length > 0)
        WGPU_SHADER_TOY.onFile(e.target.files[0]);
    }
    dialog.click();
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

  // wgpu_shader_toy_import_from_url
  wgpu_shader_toy_import_from_url: (iToken, url) => {
    if(!url)
      return;

    url = UTF8ToString(url);

    async function fetchContent() {
      const response = await fetch(url);
      if (!response.ok) {
        throw new Error(`Server Error [${response.status}] ${response.statusText}`);
      }
      return await response.text();
    }

    var pathname = new URL(url).pathname;
    pathname = pathname.substring(pathname.lastIndexOf('/') + 1);

    fetchContent()
      .then(content => { WGPU_SHADER_TOY.onNewContent(iToken, pathname, content, null); })
      .catch(error => { WGPU_SHADER_TOY.onNewContent(iToken, pathname, null, error.message); });
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
