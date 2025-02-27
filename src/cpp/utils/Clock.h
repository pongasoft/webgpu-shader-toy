/*
 * Copyright (c) 2025 pongasoft
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

#ifndef WGPU_SHADER_TOY_CLOCK_H
#define WGPU_SHADER_TOY_CLOCK_H

namespace pongasoft::utils {

class Clock
{
public:
  void tickTime(double iTimeDelta)
  {
    if(!fPaused && !fManual)
    {
      fTime += iTimeDelta;
      fFrame++;
    }
  }

  void tickFrame(int iFrameCount, double iTimeDelta = 1.0/60.0)
  {
    fTime += iTimeDelta * iFrameCount;
    fFrame += iFrameCount;
    if(fTime < 0 || fFrame < 0)
      reset();
  }

  constexpr double getTime() const { return fTime; }
  constexpr int getFrame() const { return fFrame; }
  constexpr bool isRunning() const { return !fPaused; }

  void pause() { fPaused = true; }
  void resume() { fPaused = false; }

  void setManual(bool iManual) { fManual = iManual; }
  constexpr bool isManual() const { return fManual; }

  void reset()
  {
    fTime = 0;
    fFrame = 0;
  }

private:
  double fTime{};
  int fFrame{};
  bool fPaused{};
  bool fManual{};
};

}

#endif //WGPU_SHADER_TOY_CLOCK_H
