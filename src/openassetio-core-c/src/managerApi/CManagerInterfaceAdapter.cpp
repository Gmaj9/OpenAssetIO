// SPDX-License-Identifier: Apache-2.0
// Copyright 2013-2022 The Foundry Visionmongers Ltd

#include <stdexcept>
#include <string>

#include "CManagerInterfaceAdapter.hpp"

#include <openassetio/errors/exceptions.hpp>
#include "../errors.hpp"
#include "../handles/InfoDictionary.hpp"

namespace openassetio {
inline namespace OPENASSETIO_CORE_ABI_VERSION {
namespace managerApi {

constexpr size_t kStringBufferSize = 500;

CManagerInterfaceAdapter::CManagerInterfaceAdapter(oa_managerApi_CManagerInterface_h handle,
                                                   oa_managerApi_CManagerInterface_s suite)
    : handle_{handle}, suite_{suite} {}

CManagerInterfaceAdapter::~CManagerInterfaceAdapter() { suite_.dtor(handle_); }

Identifier CManagerInterfaceAdapter::identifier() const {
  // Buffer for error message.
  char errorMessageBuffer[kStringBufferSize];
  // Error message.
  oa_StringView errorMessage{kStringBufferSize, static_cast<char*>(errorMessageBuffer), 0};

  // Return value string buffer.
  char outBuffer[kStringBufferSize];
  // Return value.
  oa_StringView out{kStringBufferSize, static_cast<char*>(outBuffer), 0};

  // Execute corresponding suite function.
  const oa_ErrorCode errorCode = suite_.identifier(&errorMessage, &out, handle_);

  // Convert error code/message to exception.
  errors::throwIfError(errorCode, errorMessage);

  return {out.data, out.size};
}

Str CManagerInterfaceAdapter::displayName() const {
  // Buffer for error message.
  char errorMessageBuffer[kStringBufferSize];
  // Error message.
  oa_StringView errorMessage{kStringBufferSize, static_cast<char*>(errorMessageBuffer), 0};

  // Return value string buffer.
  char outBuffer[kStringBufferSize];
  // Return value.
  oa_StringView out{kStringBufferSize, static_cast<char*>(outBuffer), 0};

  // Execute corresponding suite function.
  const oa_ErrorCode errorCode = suite_.displayName(&errorMessage, &out, handle_);

  // Convert error code/message to exception.
  errors::throwIfError(errorCode, errorMessage);

  return {out.data, out.size};
}

InfoDictionary CManagerInterfaceAdapter::info() {
  // Buffer for error message.
  char errorMessageBuffer[kStringBufferSize];
  // Error message.
  oa_StringView errorMessage{kStringBufferSize, static_cast<char*>(errorMessageBuffer), 0};

  // Return value.
  InfoDictionary infoDict{};
  oa_InfoDictionary_h infoDictHandle = handles::InfoDictionary::toHandle(&infoDict);

  // Execute corresponding suite function.
  const oa_ErrorCode errorCode = suite_.info(&errorMessage, infoDictHandle, handle_);

  // Convert error code/message to exception.
  errors::throwIfError(errorCode, errorMessage);

  return infoDict;
}

bool CManagerInterfaceAdapter::hasCapability([[maybe_unused]] Capability capability) {
  throw errors::NotImplementedException{"Not implemented"};
}

void CManagerInterfaceAdapter::initialize([[maybe_unused]] InfoDictionary managerSettings,
                                          [[maybe_unused]] const HostSessionPtr& hostSession) {
  throw errors::NotImplementedException{"Not implemented"};
}

trait::TraitsDatas CManagerInterfaceAdapter::managementPolicy(
    [[maybe_unused]] const trait::TraitSets& traitSets,
    [[maybe_unused]] const access::PolicyAccess policyAccess,
    [[maybe_unused]] const ContextConstPtr& context,
    [[maybe_unused]] const HostSessionPtr& hostSession) {
  throw errors::NotImplementedException{"Not implemented"};
}

bool CManagerInterfaceAdapter::isEntityReferenceString(
    [[maybe_unused]] const Str& someString, [[maybe_unused]] const HostSessionPtr& hostSession) {
  throw errors::NotImplementedException{"Not implemented"};
}

void CManagerInterfaceAdapter::entityExists(
    [[maybe_unused]] const EntityReferences& entityReferences,
    [[maybe_unused]] const ContextConstPtr& context,
    [[maybe_unused]] const HostSessionPtr& hostSession,
    [[maybe_unused]] const ManagerInterface::ExistsSuccessCallback& successCallback,
    [[maybe_unused]] const ManagerInterface::BatchElementErrorCallback& errorCallback) {
  throw errors::NotImplementedException{"Not implemented"};
}

void CManagerInterfaceAdapter::resolve(
    [[maybe_unused]] const EntityReferences& entityReferences,
    [[maybe_unused]] const trait::TraitSet& traitSet,
    [[maybe_unused]] const access::ResolveAccess resolveAccess,
    [[maybe_unused]] const ContextConstPtr& context,
    [[maybe_unused]] const HostSessionPtr& hostSession,
    [[maybe_unused]] const ManagerInterface::ResolveSuccessCallback& successCallback,
    [[maybe_unused]] const BatchElementErrorCallback& errorCallback) {
  throw errors::NotImplementedException{"Not implemented"};
}

void CManagerInterfaceAdapter::preflight(
    [[maybe_unused]] const EntityReferences& entityReferences,
    [[maybe_unused]] const trait::TraitsDatas& traitsDatas,
    [[maybe_unused]] const access::PublishingAccess publishingAccess,
    [[maybe_unused]] const ContextConstPtr& context,
    [[maybe_unused]] const HostSessionPtr& hostSession,
    [[maybe_unused]] const PreflightSuccessCallback& successCallback,
    [[maybe_unused]] const BatchElementErrorCallback& errorCallback) {
  throw errors::NotImplementedException{"Not implemented"};
}

void CManagerInterfaceAdapter::register_(
    [[maybe_unused]] const EntityReferences& entityReferences,
    [[maybe_unused]] const trait::TraitsDatas& traitsDatas,
    [[maybe_unused]] const access::PublishingAccess publishingAccess,
    [[maybe_unused]] const ContextConstPtr& context,
    [[maybe_unused]] const HostSessionPtr& hostSession,
    [[maybe_unused]] const RegisterSuccessCallback& successCallback,
    [[maybe_unused]] const BatchElementErrorCallback& errorCallback) {
  throw errors::NotImplementedException{"Not implemented"};
}

}  // namespace managerApi
}  // namespace OPENASSETIO_CORE_ABI_VERSION
}  // namespace openassetio
