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
    Module["wgpuShaderToyLoadFragmentShader"] = WGPU_SHADER_TOY.loadFragmentShader;
    `,
  $WGPU_SHADER_TOY: {
    fNewFragmentShaderHandler: null, // {handler: <fn(userData, filename, content)>, userData: <ptr>}


    // onNewFragmentShader
    onNewFragmentShader: (fragmentShaderFile, fragmentShaderContent) => {
      if(WGPU_SHADER_TOY.fNewFragmentShaderHandler) {
        const filename = stringToNewUTF8(fragmentShaderFile.name);
        const content = stringToNewUTF8(fragmentShaderContent);
        {{{ makeDynCall('vppp', 'WGPU_SHADER_TOY.fNewFragmentShaderHandler.handler') }}}(WGPU_SHADER_TOY.fNewFragmentShaderHandler.userData, filename, content);
        _free(content);
        _free(filename);
      }
    },

    // loadFragmentShader
    loadFragmentShader: (file) => {
      let reader = new FileReader();
      reader.onload = function(evt) {
        WGPU_SHADER_TOY.onNewFragmentShader(file, evt.target.result);
      };
      reader.readAsText(file);
    },

    // createFragmentShaderFileDialog
    createFragmentShaderFileDialog: () => {
      WGPU_SHADER_TOY.fFragmentShaderFileDialog = document.createElement("input");
      WGPU_SHADER_TOY.fFragmentShaderFileDialog.type = 'file';
      WGPU_SHADER_TOY.fFragmentShaderFileDialog.onchange = (e) => {
        WGPU_SHADER_TOY.loadFragmentShader(e.target.files[0]);
        // implementation note: loading the same file twice does not work unless I recreate the input...
        WGPU_SHADER_TOY.createFragmentShaderFileDialog();
      }
    }
  },

  // wgpu_shader_toy_install_new_fragment_shader_handler
  wgpu_shader_toy_install_new_fragment_shader_handler: (handler, user_data) => {
    WGPU_SHADER_TOY.fNewFragmentShaderHandler = {
      handler: handler,
      userData: user_data
    }
    WGPU_SHADER_TOY.createFragmentShaderFileDialog();
  },

  // wgpu_shader_toy_uninstall_new_fragment_shader_handler
  wgpu_shader_toy_uninstall_new_fragment_shader_handler: () => {
    delete WGPU_SHADER_TOY.fFragmentShaderFileDialog;
    delete WGPU_SHADER_TOY.fNewFragmentShaderHandler;
  },

  // wgpu_shader_toy_open_file_dialog
  wgpu_shader_toy_open_file_dialog: () => {
    console.log('wgpu_shader_toy_open_file_dialog()');
    if(WGPU_SHADER_TOY.fFragmentShaderFileDialog) {
      console.log('wgpu_shader_toy_open_file_dialog.click()');
      WGPU_SHADER_TOY.fFragmentShaderFileDialog.click();
    }
  }
}

autoAddDeps(wgpu_shader_toy, '$WGPU_SHADER_TOY')
mergeInto(LibraryManager.library, wgpu_shader_toy);
