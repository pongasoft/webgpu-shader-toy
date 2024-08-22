/*
 * Copyright (c) 2022 pongasoft
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

#include "UndoManager.h"
#include "../Errors.h"

#include <ranges>

namespace pongasoft::utils {

namespace stl {

//------------------------------------------------------------------------
// stl::last
//------------------------------------------------------------------------
inline Action *last(std::vector<std::unique_ptr<Action>> const &v)
{
  if(v.empty())
    return {};

  auto iter = v.end() - 1;
  return iter->get();
}

//------------------------------------------------------------------------
// stl::ContainerLike
//------------------------------------------------------------------------
template<typename T>
concept ContainerLike = requires(T a) {
  typename T::value_type;
  { a.empty() } -> std::convertible_to<bool>;
  { a.end() } -> std::same_as<typename T::iterator>;
  { a.erase(a.end()) } -> std::same_as<typename T::iterator>;
};

//------------------------------------------------------------------------
// stl::popLastOrDefault
// return C::value_type{} on empty
//------------------------------------------------------------------------
template<ContainerLike C>
inline typename C::value_type popLastOrDefault(C &v)
{
  if(v.empty())
    return {};

  auto iter = v.end() - 1;
  auto res = std::move(*iter);
  v.erase(iter);
  return res;
}

}

//------------------------------------------------------------------------
// UndoManager::addOrMerge
//------------------------------------------------------------------------
void UndoManager::addOrMerge(std::unique_ptr<Action> iAction)
{
  if(!isEnabled())
    return;

  if(fNextUndoActionDescription)
  {
    iAction->setDescription(*fNextUndoActionDescription);
    fNextUndoActionDescription.reset();
  }

  if(!iAction->getMergeKey().empty())
  {
    if(fUndoTx)
    {
      fUndoTx->addAction(std::move(iAction));
      return;
    }
    auto last = getLastUndoAction();
    if(last && last->fMergeKey == iAction->getMergeKey())
    {
      iAction = last->merge(std::move(iAction));
      if(iAction)
      {
        if(dynamic_cast<NoOpAction *>(iAction.get()))
        {
          // merge resulted in a noop => discard last
          popLastUndoAction();
          return;
        }
        else
        {
          // merge did not happen => new undo
          addAction(std::move(iAction));
          return;
        }
      }
      else
      {
        // merge was successful => last was updated so no need to do anything
        return;
      }
    }
  }

  if(fUndoTx)
    fUndoTx->addAction(std::move(iAction));
  else
    addAction(std::move(iAction));
}

//------------------------------------------------------------------------
// UndoManager::resetMergeKey
//------------------------------------------------------------------------
void UndoManager::resetMergeKey()
{
  auto last = getLastUndoAction();
  if(last)
    last->resetMergeKey();
}

//------------------------------------------------------------------------
// UndoManager::addAction
//------------------------------------------------------------------------
void UndoManager::addAction(std::unique_ptr<Action> iAction)
{
  auto last = stl::last(fUndoHistory);
  if(last)
    last->resetMergeKey();
  fUndoHistory.emplace_back(std::move(iAction));

  fRedoHistory.clear();
}

//------------------------------------------------------------------------
// UndoManager::undoLastAction
//------------------------------------------------------------------------
void UndoManager::undoLastAction()
{
  auto action = popLastUndoAction();
  if(action)
  {
    action->resetMergeKey();
    action->undo();
    fRedoHistory.emplace_back(std::move(action));
  }
}

//------------------------------------------------------------------------
// UndoManager::redoLastAction
//------------------------------------------------------------------------
void UndoManager::redoLastAction()
{
  auto action = stl::popLastOrDefault(fRedoHistory);
  if(action)
  {
    action->redo();
    fUndoHistory.emplace_back(std::move(action));
  }
}

//------------------------------------------------------------------------
// UndoManager::getLastUndoAction
//------------------------------------------------------------------------
Action *UndoManager::getLastUndoAction() const
{
  return stl::last(fUndoHistory);
}

//------------------------------------------------------------------------
// UndoManager::getLastUndoAction
//------------------------------------------------------------------------
Action *UndoManager::getLastRedoAction() const
{
  return stl::last(fRedoHistory);
}

//------------------------------------------------------------------------
// UndoManager::popLastUndoAction
//------------------------------------------------------------------------
std::unique_ptr<Action> UndoManager::popLastUndoAction()
{
  if(!isEnabled())
    return nullptr;

  return stl::popLastOrDefault(fUndoHistory);
}

//------------------------------------------------------------------------
// UndoManager::clear
//------------------------------------------------------------------------
void UndoManager::clear()
{
  fUndoHistory.clear();
  fRedoHistory.clear();
}

//------------------------------------------------------------------------
// UndoManager::undoUntil
//------------------------------------------------------------------------
void UndoManager::undoUntil(Action const *iAction)
{
  while(!fUndoHistory.empty() && getLastUndoAction() != iAction)
    undoLastAction();
}

//------------------------------------------------------------------------
// UndoManager::undoAll
//------------------------------------------------------------------------
void UndoManager::undoAll()
{
  while(!fUndoHistory.empty())
    undoLastAction();
}

//------------------------------------------------------------------------
// UndoManager::redoUntil
//------------------------------------------------------------------------
void UndoManager::redoUntil(Action const *iAction)
{
  while(!fRedoHistory.empty() && getLastRedoAction() != iAction)
    redoLastAction();
  redoLastAction(); // we need to get one more
}

//------------------------------------------------------------------------
// UndoManager::beginTx
//------------------------------------------------------------------------
void UndoManager::beginTx(std::string iDescription, MergeKey const &iMergeKey)
{
  if(fUndoTx)
    fNestedUndoTxs.emplace_back(std::move(fUndoTx));

  fUndoTx = std::make_unique<UndoTx>(std::move(iDescription), iMergeKey);

  if(fNextUndoActionDescription)
  {
    fUndoTx->setDescription(*fNextUndoActionDescription);
    fNextUndoActionDescription.reset();
  }
}

//------------------------------------------------------------------------
// UndoManager::commitTx
//------------------------------------------------------------------------
void UndoManager::commitTx()
{
  WST_INTERNAL_ASSERT(fUndoTx != nullptr, "no current transaction");

  auto action = std::move(fUndoTx);

  if(!fNestedUndoTxs.empty())
  {
    auto iter = fNestedUndoTxs.end() - 1;
    fUndoTx = std::move(*iter);
    fNestedUndoTxs.erase(iter);
  }

  if(isEnabled())
  {
    if(action->isEmpty())
      return;

    if(auto singleAction = action->single())
      addOrMerge(std::move(singleAction));
    else
      addOrMerge(std::move(action));
  }
}

//------------------------------------------------------------------------
// UndoManager::rollbackTx
//------------------------------------------------------------------------
void UndoManager::rollbackTx()
{
  WST_INTERNAL_ASSERT(fUndoTx != nullptr, "no current transaction");

  auto action = std::move(fUndoTx);

  action->undo();

  if(!fNestedUndoTxs.empty())
  {
    auto iter = fNestedUndoTxs.end() - 1;
    fUndoTx = std::move(*iter);
    fNestedUndoTxs.erase(iter);
  }
}

//------------------------------------------------------------------------
// UndoManager::setNextActionDescription
//------------------------------------------------------------------------
void UndoManager::setNextActionDescription(std::string iDescription)
{
  if(isEnabled() && !fNextUndoActionDescription)
    fNextUndoActionDescription.emplace(std::move(iDescription));
}

//------------------------------------------------------------------------
// Action::merge
//------------------------------------------------------------------------
std::unique_ptr<Action> Action::merge(std::unique_ptr<Action> iAction)
{
  if(fMergeKey.empty() ||
     iAction->getMergeKey().empty() ||
     fMergeKey != iAction->getMergeKey() ||
     !canMergeWith(iAction.get()))
  {
    return std::move(iAction);
  }

  return doMerge(std::move(iAction));
}

//------------------------------------------------------------------------
// CompositeAction::undo
//------------------------------------------------------------------------
void CompositeAction::undo()
{
  // reverse order!
  for(auto &fAction : std::ranges::reverse_view(fActions))
  {
    fAction->undo();
  }
}

//------------------------------------------------------------------------
// CompositeAction::redo
//------------------------------------------------------------------------
void CompositeAction::redo()
{
  for(auto &action: fActions)
    action->redo();
}

//------------------------------------------------------------------------
// UndoTx::UndoTx
//------------------------------------------------------------------------
UndoTx::UndoTx(std::string iDescription, MergeKey const &iMergeKey)
{
  fDescription = std::move(iDescription);
  fMergeKey = iMergeKey;
}

//------------------------------------------------------------------------
// UndoTx::single
//------------------------------------------------------------------------
std::unique_ptr<Action> UndoTx::single()
{
  if(fActions.size() != 1)
    return nullptr;

  auto res = std::move(fActions[0]);
  fActions.clear();
  return res;
}

//------------------------------------------------------------------------
// UndoTx::addAction
//------------------------------------------------------------------------
void UndoTx::addAction(std::unique_ptr<Action> iAction)
{
  fActions.emplace_back(std::move(iAction));
}

}