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

#ifndef WGPU_SHADER_TOY_FMT_H
#define WGPU_SHADER_TOY_FMT_H

#include <string>
#include <stdexcept>

namespace fmt {

namespace impl {

template<typename T>
constexpr auto printf_arg(T const &t) { return t; }

// Handles std::string without having to call c_str all the time
template<>
inline auto printf_arg<std::string>(std::string const &s) { return s.c_str(); }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif
/*
 * Copied from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf */
template<typename ... Args>
std::string printf(const std::string& format, Args ... args )
{
  int size_s = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
  if(size_s <= 0) { throw std::runtime_error("Error during formatting."); }
  auto size = static_cast<size_t>( size_s );
  auto buf = std::make_unique<char[]>(size);
  snprintf(buf.get(), size_s, format.c_str(), args ...);
  return {buf.get(), buf.get() + size - 1}; // We don't want the '\0' inside
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

}

//------------------------------------------------------------------------
// printf -> std::string
//------------------------------------------------------------------------
template<typename ... Args>
inline std::string printf(const std::string& format, Args ... args)
{
  return impl::printf(format, impl::printf_arg(args)...);
}

}

#endif //WGPU_SHADER_TOY_FMT_H
