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

#ifndef WGPU_SHADER_TOY_DATA_MANAGER_H
#define WGPU_SHADER_TOY_DATA_MANAGER_H

#include <vector>

namespace pongasoft::utils {

class DataManager {
public:
  static std::vector<unsigned char> loadCompressedBase85(char const *iCompressedBase85);
};

}

#endif //WGPU_SHADER_TOY_DATA_MANAGER_H