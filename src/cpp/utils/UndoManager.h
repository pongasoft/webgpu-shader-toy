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

#ifndef WGPU_SHADER_TOY_UTILS_UNDO_MANAGER_H
#define WGPU_SHADER_TOY_UTILS_UNDO_MANAGER_H

#include <string>
#include <functional>
#include <vector>
#include <concepts>

namespace pongasoft::utils {

class Action
{
public:
  virtual ~Action() = default;
  virtual void undo() = 0;
  virtual void redo() = 0;

  std::string const &getDescription() const { return fDescription; }
  void setDescription(std::string iDescription) { fDescription = std::move(iDescription); }

public:
  std::string fDescription{};
};


template<typename A>
concept IsAction = std::derived_from<A, Action>;

class NoOpAction : public Action
{
public:
  void undo() override {}
  void redo() override {}
  static std::unique_ptr<NoOpAction> create() { return std::make_unique<NoOpAction>(); }
};

class CompositeAction : public Action
{
public:
  void undo() override;
  void redo() override;

  inline bool isEmpty() const { return fActions.empty(); }
  inline auto getSize() const { return fActions.size(); }

  std::vector<std::unique_ptr<Action>> const &getActions() const { return fActions; }

protected:
  std::vector<std::unique_ptr<Action>> fActions{};
};

class UndoTx : public CompositeAction
{
public:
  UndoTx(std::string iDescription);
  std::unique_ptr<Action> single();
  void addAction(std::unique_ptr<Action> iAction);
};

template<typename R, IsAction A = Action>
class ExecutableAction : public A
{
public:
  using result_t = R;
  using action_t = A;

  virtual result_t execute() = 0;

  inline bool isUndoEnabled() const { return fUndoEnabled; }

  void redo() override
  {
    execute();
  }

protected:
  bool fUndoEnabled{true};
};


class UndoManager
{
public:
  constexpr bool isEnabled() const { return fEnabled; }
  constexpr void enable() { fEnabled = true; }
  constexpr void disable() { fEnabled = false; }
  void addOrMerge(std::unique_ptr<Action> iAction);
  void beginTx(std::string iDescription);
  void commitTx();
  void rollbackTx();
  void setNextActionDescription(std::string iDescription);
  void undoLastAction();
  void undoUntil(Action const *iAction);
  void undoAll();
  void redoLastAction();
  void redoUntil(Action const *iAction);
  inline bool hasUndoHistory() const { return !fUndoHistory.empty(); }
  inline bool hasRedoHistory() const { return !fRedoHistory.empty(); }
  inline bool hasHistory() const { return hasUndoHistory() || hasRedoHistory(); }
  std::unique_ptr<Action> popLastUndoAction();
  Action *getLastUndoAction() const;
  Action *getLastRedoAction() const;
  std::vector<std::unique_ptr<Action>> const &getUndoHistory() const { return fUndoHistory; }
  std::vector<std::unique_ptr<Action>> const &getRedoHistory() const { return fRedoHistory; }
  void clear();

  template<typename R, IsAction A = Action>
  R execute(std::unique_ptr<ExecutableAction<R, A>> iAction);

  template<class T, class... Args >
  typename T::result_t executeAction(Args&&... args);

  template<class T, class... Args >
  static std::unique_ptr<T> createAction(Args&&... args);

protected:
  void addAction(std::unique_ptr<Action> iAction);

private:
  bool fEnabled{true};
  std::unique_ptr<UndoTx> fUndoTx{};
  std::vector<std::unique_ptr<UndoTx>> fNestedUndoTxs{};
  std::optional<std::string> fNextUndoActionDescription{};
  std::vector<std::unique_ptr<Action>> fUndoHistory{};
  std::vector<std::unique_ptr<Action>> fRedoHistory{};
};

//------------------------------------------------------------------------
// UndoManager::execute
//------------------------------------------------------------------------
template<typename R, IsAction A>
R UndoManager::execute(std::unique_ptr<ExecutableAction<R, A>> iAction)
{
  if constexpr (std::is_void_v<R>)
  {
    iAction->execute();
    if(isEnabled() && iAction->isUndoEnabled())
    {
      addOrMerge(std::move(iAction));
    }
  }
  else
  {
    auto &&result = iAction->execute();
    if(isEnabled() && iAction->isUndoEnabled())
    {
      addOrMerge(std::move(iAction));
    }
    return result;
  }
}

//------------------------------------------------------------------------
// UndoManager::createAction
//------------------------------------------------------------------------
template<class T, class... Args>
std::unique_ptr<T> UndoManager::createAction(Args &&... args)
{
  auto action = std::make_unique<T>();
  action->init(std::forward<Args>(args)...);
  return action;
}


//------------------------------------------------------------------------
// UndoManager::executeAction
//------------------------------------------------------------------------
template<class T, class... Args>
typename T::result_t UndoManager::executeAction(Args &&... args)
{
  return execute<typename T::result_t, typename T::action_t>(createAction<T>(std::forward<Args>(args)...));
}



}



#endif //WGPU_SHADER_TOY_UTILS_UNDO_MANAGER_H
