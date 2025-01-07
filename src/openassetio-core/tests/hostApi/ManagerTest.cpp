// SPDX-License-Identifier: Apache-2.0
// Copyright 2022-2025 The Foundry Visionmongers Ltd
#include <cstddef>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include <openassetio/export.h>

#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>
#include <trompeloeil.hpp>

#include <openassetio/Context.hpp>
#include <openassetio/EntityReference.hpp>
#include <openassetio/access.hpp>
#include <openassetio/errors/BatchElementError.hpp>
#include <openassetio/errors/exceptions.hpp>
#include <openassetio/hostApi/HostInterface.hpp>
#include <openassetio/hostApi/Manager.hpp>
#include <openassetio/log/LoggerInterface.hpp>
#include <openassetio/managerApi/Host.hpp>
#include <openassetio/managerApi/HostSession.hpp>
#include <openassetio/managerApi/ManagerInterface.hpp>
#include <openassetio/trait/TraitsData.hpp>
#include <openassetio/trait/collection.hpp>

namespace openassetio {
inline namespace OPENASSETIO_CORE_ABI_VERSION {
namespace {
/**
 * Mock implementation of a ManagerInterface.
 *
 * Used as constructor parameter to the Manager under test.
 */
struct MockManagerInterface final : trompeloeil::mock_interface<managerApi::ManagerInterface> {
  IMPLEMENT_CONST_MOCK0(identifier);
  IMPLEMENT_CONST_MOCK0(displayName);
  IMPLEMENT_MOCK0(info);
  IMPLEMENT_MOCK2(initialize);
  IMPLEMENT_MOCK1(hasCapability);
  IMPLEMENT_MOCK4(managementPolicy);
  IMPLEMENT_MOCK2(isEntityReferenceString);
  IMPLEMENT_MOCK5(entityExists);
  IMPLEMENT_MOCK7(resolve);
  IMPLEMENT_MOCK7(preflight);
  IMPLEMENT_MOCK7(register_);  // NOLINT(readability-identifier-naming)
};
/**
 * Mock implementation of a HostInterface.
 *
 * Used as constructor parameter to Host classes required as part of these tests
 */
struct MockHostInterface final : trompeloeil::mock_interface<hostApi::HostInterface> {
  IMPLEMENT_CONST_MOCK0(identifier);
  IMPLEMENT_CONST_MOCK0(displayName);
  IMPLEMENT_MOCK0(info);
};
/**
 * Mock implementation of a LoggerInterface
 *
 * Used as constructor parameter to Host classes required as part of these tests
 */
struct MockLoggerInterface final : trompeloeil::mock_interface<log::LoggerInterface> {
  IMPLEMENT_MOCK2(log);
};

/**
 * Fixture providing a Manager instance injected with mock dependencies.
 */
// NOLINTBEGIN(*-avoid-const-or-ref-data-members)
struct ManagerFixture {
  const std::shared_ptr<managerApi::ManagerInterface> managerInterface =
      std::make_shared<MockManagerInterface>();

  // For convenience, to avoid casting all the time in tests.
  MockManagerInterface& mockManagerInterface =
      dynamic_cast<MockManagerInterface&>(*managerInterface);

  // Create a HostSession with our mock HostInterface
  const managerApi::HostSessionPtr hostSession =
      managerApi::HostSession::make(managerApi::Host::make(std::make_shared<MockHostInterface>()),
                                    std::make_shared<MockLoggerInterface>());

  // Create the Manager under test.
  const hostApi::ManagerPtr manager = hostApi::Manager::make(managerInterface, hostSession);

  // For convenience, since almost every method takes a Context.
  const ContextPtr context{Context::make()};
};
// NOLINTEND(*-avoid-const-or-ref-data-members)

}  // namespace
}  // namespace OPENASSETIO_CORE_ABI_VERSION
}  // namespace openassetio

namespace {
/* Oft-reused predicate to check if an error matches an exception by
code and message*/
auto makeErrorExceptionMatchPredicate(const openassetio::errors::BatchElementError& expected) {
  return Catch::Predicate<openassetio::errors::BatchElementException>(
      [&expected](const openassetio::errors::BatchElementException& exc) -> bool {
        return exc.error.code == expected.code &&
               std::string(exc.what()).find(expected.message) != std::string::npos;
      },
      "Thrown exception has unexpected message or code");
}
}  // namespace

SCENARIO("Manager constructor is private") {
  STATIC_REQUIRE_FALSE(std::is_constructible_v<openassetio::hostApi::Manager,
                                               openassetio::managerApi::ManagerInterfacePtr>);
}

SCENARIO("Resolving entities") {
  namespace hostApi = openassetio::hostApi;
  using trompeloeil::_;

  GIVEN("a configured Manager instance") {
    const openassetio::trait::TraitSet traits = {"fakeTrait", "secondFakeTrait"};
    const openassetio::ManagerFixture fixture;
    const auto& manager = fixture.manager;
    auto& mockManagerInterface = fixture.mockManagerInterface;
    const auto& context = fixture.context;
    const auto& hostSession = fixture.hostSession;
    constexpr auto kResolveAccess = openassetio::access::ResolveAccess::kRead;

    GIVEN("manager plugin successfully resolves a single entity reference") {
      const openassetio::EntityReference ref{"testReference"};
      const openassetio::EntityReferences refs = {ref};

      const openassetio::trait::TraitsDataPtr expected = openassetio::trait::TraitsData::make();
      expected->addTrait("aTestTrait");

      // With success callback side effect
      REQUIRE_CALL(mockManagerInterface,
                   resolve(refs, traits, kResolveAccess, context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(0, expected));

      WHEN("singular resolve is called with default errorPolicyTag") {
        const openassetio::trait::TraitsDataPtr actual =
            manager->resolve(ref, traits, kResolveAccess, context);
        THEN("returned TraitsData is as expected") { CHECK(expected.get() == actual.get()); }
      }
      WHEN("singular resolve is called with kException errorPolicyTag") {
        const openassetio::trait::TraitsDataPtr actual =
            manager->resolve(ref, traits, kResolveAccess, context,
                             hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned TraitsData is as expected") { CHECK(expected.get() == actual.get()); }
      }
      WHEN("singular resolve is called with kVariant errorPolicyTag") {
        std::variant<openassetio::errors::BatchElementError, openassetio::trait::TraitsDataPtr>
            actual = manager->resolve(ref, traits, kResolveAccess, context,
                                      hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned variant contains the expected TraitsData") {
          CHECK(std::holds_alternative<openassetio::trait::TraitsDataPtr>(actual));
          auto actualVal = std::get<openassetio::trait::TraitsDataPtr>(actual);
          CHECK(expected.get() == actualVal.get());
        }
      }
    }
    GIVEN("manager plugin successfully resolves multiple entity references") {
      const openassetio::EntityReferences refs = {openassetio::EntityReference{"testReference1"},
                                                  openassetio::EntityReference{"testReference2"},
                                                  openassetio::EntityReference{"testReference3"}};

      const openassetio::trait::TraitsDataPtr expected1 = openassetio::trait::TraitsData::make();
      expected1->addTrait("aTestTrait1");
      const openassetio::trait::TraitsDataPtr expected2 = openassetio::trait::TraitsData::make();
      expected2->addTrait("aTestTrait2");
      const openassetio::trait::TraitsDataPtr expected3 = openassetio::trait::TraitsData::make();
      expected3->addTrait("aTestTrait3");
      std::vector expectedVec{expected1, expected2, expected3};

      // With success callback side effect
      REQUIRE_CALL(mockManagerInterface,
                   resolve(refs, traits, kResolveAccess, context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(0, expectedVec[0]))
          .LR_SIDE_EFFECT(_6(1, expectedVec[1]))
          .LR_SIDE_EFFECT(_6(2, expectedVec[2]));

      WHEN("batch resolve is called with default errorPolicyTag") {
        const std::vector<openassetio::trait::TraitsDataPtr> actualVec =
            manager->resolve(refs, traits, kResolveAccess, context);
        THEN("returned list of TraitsDatas is as expected") { CHECK(expectedVec == actualVec); }
      }
      WHEN("batch resolve is called with kException errorPolicyTag") {
        const std::vector<openassetio::trait::TraitsDataPtr> actualVec =
            manager->resolve(refs, traits, kResolveAccess, context,
                             hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned list of TraitsDatas is as expected") { CHECK(expectedVec == actualVec); }
      }
      WHEN("batch resolve is called with kVariant errorPolicyTag") {
        std::vector<std::variant<openassetio::errors::BatchElementError,
                                 openassetio::trait::TraitsDataPtr>>
            actualVec = manager->resolve(refs, traits, kResolveAccess, context,
                                         hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants contains the expected TraitsDatas") {
          CHECK(expectedVec.size() == actualVec.size());
          for (std::size_t i = 0; i < actualVec.size(); ++i) {
            CHECK(std::holds_alternative<openassetio::trait::TraitsDataPtr>(actualVec[i]));
            auto actualVal = std::get<openassetio::trait::TraitsDataPtr>(actualVec[i]);
            CHECK(expectedVec[i].get() == actualVal.get());
          }
        }
      }
    }
    GIVEN("manager plugin successfully resolves multiple entity references in a non-index order") {
      const openassetio::EntityReferences refs = {openassetio::EntityReference{"testReference1"},
                                                  openassetio::EntityReference{"testReference2"},
                                                  openassetio::EntityReference{"testReference3"}};

      const openassetio::trait::TraitsDataPtr expected1 = openassetio::trait::TraitsData::make();
      expected1->addTrait("aTestTrait1");
      const openassetio::trait::TraitsDataPtr expected2 = openassetio::trait::TraitsData::make();
      expected2->addTrait("aTestTrait2");
      const openassetio::trait::TraitsDataPtr expected3 = openassetio::trait::TraitsData::make();
      expected3->addTrait("aTestTrait3");
      std::vector expectedVec{expected1, expected2, expected3};

      // With success callback side effect, given out of order.
      REQUIRE_CALL(mockManagerInterface,
                   resolve(refs, traits, kResolveAccess, context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(2, expectedVec[2]))
          .LR_SIDE_EFFECT(_6(0, expectedVec[0]))
          .LR_SIDE_EFFECT(_6(1, expectedVec[1]));

      WHEN("batch resolve is called with default errorPolicyTag") {
        const std::vector<openassetio::trait::TraitsDataPtr> actualVec =
            manager->resolve(refs, traits, kResolveAccess, context);
        THEN("returned list of TraitsDatas is ordered in index order") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch resolve is called with kException errorPolicyTag") {
        const std::vector<openassetio::trait::TraitsDataPtr> actualVec =
            manager->resolve(refs, traits, kResolveAccess, context,
                             hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned list of TraitsDatas is ordered in index order") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch resolve is called with kVariant errorPolicyTag") {
        std::vector<std::variant<openassetio::errors::BatchElementError,
                                 openassetio::trait::TraitsDataPtr>>
            actualVec = manager->resolve(refs, traits, kResolveAccess, context,
                                         hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants is ordered in index order") {
          CHECK(expectedVec.size() == actualVec.size());
          for (std::size_t i = 0; i < actualVec.size(); ++i) {
            CHECK(std::holds_alternative<openassetio::trait::TraitsDataPtr>(actualVec[i]));
            auto actualVal = std::get<openassetio::trait::TraitsDataPtr>(actualVec[i]);
            CHECK(expectedVec[i].get() == actualVal.get());
          }
        }
      }
    }
    GIVEN(
        "manager plugin will encounter an entity-specific error when next resolving a reference") {
      const openassetio::EntityReference ref{"testReference"};
      const openassetio::EntityReferences refs = {ref};

      openassetio::errors::BatchElementError expected{
          openassetio::errors::BatchElementError::ErrorCode::kMalformedEntityReference,
          "Error Message"};

      // With error callback side effect
      REQUIRE_CALL(mockManagerInterface,
                   resolve(refs, traits, kResolveAccess, context, hostSession, _, _))
          .LR_SIDE_EFFECT(_7(0, expected));

      WHEN("singular resolve is called with default errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(manager->resolve(ref, traits, kResolveAccess, context),
                               openassetio::errors::BatchElementException,
                               makeErrorExceptionMatchPredicate(expected));
        }
      }
      WHEN("singular resolve is called with kException errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->resolve(ref, traits, kResolveAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expected));
        }
      }
      WHEN("singular resolve is called with kVariant errorPolicyTag") {
        std::variant<openassetio::errors::BatchElementError, openassetio::trait::TraitsDataPtr>
            actual = manager->resolve(ref, traits, kResolveAccess, context,
                                      hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned variant contains the expected BatchElementError") {
          CHECK(std::holds_alternative<openassetio::errors::BatchElementError>(actual));
          const auto& actualVal = std::get<openassetio::errors::BatchElementError>(actual);
          CHECK(expected == actualVal);
        }
      }
    }
    GIVEN(
        "manager plugin will encounter entity-specific errors when next resolving multiple "
        "references") {
      const openassetio::EntityReferences refs = {openassetio::EntityReference{"testReference1"},
                                                  openassetio::EntityReference{"testReference2"},
                                                  openassetio::EntityReference{"testReference3"}};

      const openassetio::trait::TraitsDataPtr expectedValue2 =
          openassetio::trait::TraitsData::make();
      expectedValue2->addTrait("aTestTrait");
      const openassetio::errors::BatchElementError expectedError0{
          openassetio::errors::BatchElementError::ErrorCode::kMalformedEntityReference,
          "Malformed Mock Error🤖"};
      const openassetio::errors::BatchElementError expectedError1{
          openassetio::errors::BatchElementError::ErrorCode::kEntityAccessError,
          "Entity Access Error Message"};

      // With mixed callback side effect
      REQUIRE_CALL(mockManagerInterface,
                   resolve(refs, traits, kResolveAccess, context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(2, expectedValue2))
          .LR_SIDE_EFFECT(_7(0, expectedError0))
          .LR_SIDE_EFFECT(_7(1, expectedError1));

      WHEN("batch resolve is called with default errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(manager->resolve(refs, traits, kResolveAccess, context),
                               openassetio::errors::BatchElementException,
                               makeErrorExceptionMatchPredicate(expectedError0));
        }
      }
      WHEN("batch resolve is called with kException errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->resolve(refs, traits, kResolveAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expectedError0));
        }
      }
      WHEN("batch resolve is called with kVariant errorPolicyTag") {
        std::vector<std::variant<openassetio::errors::BatchElementError,
                                 openassetio::trait::TraitsDataPtr>>
            actualVec = manager->resolve(refs, traits, kResolveAccess, context,
                                         hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants contains the expected objects") {
          auto error0 = std::get<openassetio::errors::BatchElementError>(actualVec[0]);
          CHECK(error0 == expectedError0);

          auto error1 = std::get<openassetio::errors::BatchElementError>(actualVec[1]);
          CHECK(error1 == expectedError1);

          CHECK(std::get<openassetio::trait::TraitsDataPtr>(actualVec[2]) == expectedValue2);
        }
      }
    }
  }
}

using ErrorCode = openassetio::errors::BatchElementError::ErrorCode;

SCENARIO("Preflighting entities") {
  namespace hostApi = openassetio::hostApi;
  using trompeloeil::_;

  GIVEN("a configured Manager instance") {
    const openassetio::EntityReference ref{"testReference"};
    const openassetio::EntityReferences threeRefs = {
        openassetio::EntityReference{"testReference1"},
        openassetio::EntityReference{"testReference2"},
        openassetio::EntityReference{"testReference3"}};

    const openassetio::trait::TraitsDataPtr traitsData =
        openassetio::trait::TraitsData::make({"fakeTrait", "secondFakeTrait"});
    const openassetio::trait::TraitsDatas threeTraitsDatas{traitsData, traitsData, traitsData};

    const openassetio::ManagerFixture fixture;
    const auto& manager = fixture.manager;
    auto& mockManagerInterface = fixture.mockManagerInterface;
    const auto& context = fixture.context;
    const auto& hostSession = fixture.hostSession;
    constexpr auto kPublishingAccess = openassetio::access::PublishingAccess::kWrite;

    GIVEN("manager plugin successfully preflights a single entity reference") {
      const openassetio::EntityReference expected{"preflightedRef"};

      // With success callback side effect
      REQUIRE_CALL(mockManagerInterface, preflight(openassetio::EntityReferences{ref},
                                                   openassetio::trait::TraitsDatas{traitsData},
                                                   kPublishingAccess, context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(0, expected));

      WHEN("singular preflight is called with default errorPolicyTag") {
        const openassetio::EntityReference actual =
            manager->preflight(ref, traitsData, kPublishingAccess, context);
        THEN("returned entity reference is as expected") { CHECK(expected == actual); }
      }
      WHEN("singular preflight is called with kException errorPolicyTag") {
        const openassetio::EntityReference actual =
            manager->preflight(ref, traitsData, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned entity reference is as expected") { CHECK(expected == actual); }
      }
      WHEN("singular preflight is called with kVariant errorPolicyTag") {
        std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference> actual =
            manager->preflight(ref, traitsData, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned variant contains the expected entity reference") {
          CHECK(std::holds_alternative<openassetio::EntityReference>(actual));
          auto actualVal = std::get<openassetio::EntityReference>(actual);
          CHECK(expected == actualVal);
        }
      }
    }
    GIVEN("manager plugin successfully preflights multiple entity references") {
      const openassetio::EntityReference expected1{"ref1"};
      const openassetio::EntityReference expected2{"ref2"};
      const openassetio::EntityReference expected3{"ref3"};
      std::vector expectedVec{expected1, expected2, expected3};

      // With success callback side effect
      REQUIRE_CALL(mockManagerInterface, preflight(threeRefs, threeTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(0, expectedVec[0]))
          .LR_SIDE_EFFECT(_6(1, expectedVec[1]))
          .LR_SIDE_EFFECT(_6(2, expectedVec[2]));

      WHEN("batch preflight is called with default errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context);
        THEN("returned list of entity references is as expected") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch preflight is called with kException errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned list of entity references is as expected") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch preflight is called with kVariant errorPolicyTag") {
        std::vector<
            std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference>>
            actualVec = manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context,
                                           hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants contains the expected entity references") {
          CHECK(expectedVec.size() == actualVec.size());
          for (size_t i = 0; i < actualVec.size(); ++i) {
            CHECK(std::holds_alternative<openassetio::EntityReference>(actualVec[i]));
            auto actualVal = std::get<openassetio::EntityReference>(actualVec[i]);
            CHECK(expectedVec[i] == actualVal);
          }
        }
      }
    }
    GIVEN(
        "manager plugin successfully preflights multiple entity references in a non-index order") {
      const openassetio::EntityReference expected1{"ref1"};
      const openassetio::EntityReference expected2{"ref2"};
      const openassetio::EntityReference expected3{"ref3"};
      std::vector expectedVec{expected1, expected2, expected3};

      // With success callback side effect, given out of order.
      REQUIRE_CALL(mockManagerInterface, preflight(threeRefs, threeTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(2, expectedVec[2]))
          .LR_SIDE_EFFECT(_6(0, expectedVec[0]))
          .LR_SIDE_EFFECT(_6(1, expectedVec[1]));

      WHEN("batch preflight is called with default errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context);
        THEN("returned list of entity references is ordered in index order") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch preflight is called with kException errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned list of entity references is ordered in index order") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch preflight is called with kVariant errorPolicyTag") {
        std::vector<
            std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference>>
            actualVec = manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context,
                                           hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants is ordered in index order") {
          CHECK(expectedVec.size() == actualVec.size());
          for (size_t i = 0; i < actualVec.size(); ++i) {
            CHECK(std::holds_alternative<openassetio::EntityReference>(actualVec[i]));
            auto actualVal = std::get<openassetio::EntityReference>(actualVec[i]);
            CHECK(expectedVec[i] == actualVal);
          }
        }
      }
    }
    GIVEN(
        "manager plugin will encounter an entity-specific error when next preflighting a "
        "reference") {
      const openassetio::errors::BatchElementError expected{ErrorCode::kMalformedEntityReference,
                                                            "Error Message"};

      // With error callback side effect
      REQUIRE_CALL(mockManagerInterface, preflight(openassetio::EntityReferences{ref},
                                                   openassetio::trait::TraitsDatas{traitsData},
                                                   kPublishingAccess, context, hostSession, _, _))
          .LR_SIDE_EFFECT(_7(0, expected));

      WHEN("singular preflight is called with default errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(manager->preflight(ref, traitsData, kPublishingAccess, context),
                               openassetio::errors::BatchElementException,
                               makeErrorExceptionMatchPredicate(expected));
        }
      }
      WHEN("singular preflight is called with kException errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->preflight(ref, traitsData, kPublishingAccess, context,
                                 hostApi::Manager::BatchElementErrorPolicyTag::kException),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expected));
        }
      }
      WHEN("singular preflight is called with kVariant errorPolicyTag") {
        std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference> actual =
            manager->preflight(ref, traitsData, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned variant contains the expected BatchElementError") {
          CHECK(std::holds_alternative<openassetio::errors::BatchElementError>(actual));
          const auto& actualVal = std::get<openassetio::errors::BatchElementError>(actual);
          CHECK(expected == actualVal);
        }
      }
    }
    GIVEN(
        "manager plugin will encounter entity-specific errors when next preflighting multiple "
        "references") {
      const openassetio::EntityReference expectedValue2{"ref2"};
      const openassetio::errors::BatchElementError expectedError0{
          ErrorCode::kMalformedEntityReference, "Malformed Mock Error🤖"};
      const openassetio::errors::BatchElementError expectedError1{ErrorCode::kEntityAccessError,
                                                                  "Entity Access Error Message"};

      // With mixed callback side effect
      REQUIRE_CALL(mockManagerInterface, preflight(threeRefs, threeTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(2, expectedValue2))
          .LR_SIDE_EFFECT(_7(0, expectedError0))
          .LR_SIDE_EFFECT(_7(1, expectedError1));

      WHEN("batch preflight is called with default errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expectedError0));
        }
      }
      WHEN("batch preflight is called with kException errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context,
                                 hostApi::Manager::BatchElementErrorPolicyTag::kException),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expectedError0));
        }
      }
      WHEN("batch preflight is called with kVariant errorPolicyTag") {
        std::vector<
            std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference>>
            actualVec = manager->preflight(threeRefs, threeTraitsDatas, kPublishingAccess, context,
                                           hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants contains the expected objects") {
          auto error0 = std::get<openassetio::errors::BatchElementError>(actualVec[0]);
          CHECK(error0 == expectedError0);

          auto error1 = std::get<openassetio::errors::BatchElementError>(actualVec[1]);
          CHECK(error1 == expectedError1);

          CHECK(std::get<openassetio::EntityReference>(actualVec[2]) == expectedValue2);
        }
      }
    }
  }
}

SCENARIO("Registering entities") {
  namespace hostApi = openassetio::hostApi;
  using trompeloeil::_;

  GIVEN("a configured Manager instance") {
    const openassetio::ManagerFixture fixture;
    const auto& manager = fixture.manager;
    auto& mockManagerInterface = fixture.mockManagerInterface;
    const auto& context = fixture.context;
    const auto& hostSession = fixture.hostSession;
    constexpr auto kPublishingAccess = openassetio::access::PublishingAccess::kWrite;

    // Create TraitSets used in the register method calls
    const openassetio::trait::TraitSet traits = {"fakeTrait", "secondFakeTrait"};
    const openassetio::trait::TraitsDatas threeTraitsDatas = {
        openassetio::trait::TraitsData::make(traits), openassetio::trait::TraitsData::make(traits),
        openassetio::trait::TraitsData::make(traits)};

    auto singleTraitsData = openassetio::trait::TraitsData::make(traits);
    const openassetio::trait::TraitsDatas singleTraitsDatas = {singleTraitsData};

    GIVEN("manager plugin successfully registers a single entity reference") {
      const openassetio::EntityReference ref{"testReference"};
      const openassetio::EntityReferences refs = {ref};

      const openassetio::EntityReference expected{"expectedReference"};

      // With success callback side effect
      REQUIRE_CALL(mockManagerInterface, register_(refs, singleTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(0, expected));

      WHEN("singular register is called with default errorPolicyTag") {
        const openassetio::EntityReference actual =
            manager->register_(ref, singleTraitsData, kPublishingAccess, context);
        THEN("returned EntityReference is as expected") { CHECK(expected == actual); }
      }
      WHEN("singular register is called with kException errorPolicyTag") {
        const openassetio::EntityReference actual =
            manager->register_(ref, singleTraitsData, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned EntityReference is as expected") { CHECK(expected == actual); }
      }
      WHEN("singular register is called with kVariant errorPolicyTag") {
        std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference> actual =
            manager->register_(ref, singleTraitsData, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned variant contains the expected EntityReference") {
          CHECK(std::holds_alternative<openassetio::EntityReference>(actual));
          auto actualVal = std::get<openassetio::EntityReference>(actual);
          CHECK(expected == actualVal);
        }
      }
    }
    GIVEN("manager plugin successfully registers multiple entity references") {
      const openassetio::EntityReferences refs = {openassetio::EntityReference{"ref1"},
                                                  openassetio::EntityReference{"ref2"},
                                                  openassetio::EntityReference{"ref3"}};

      const openassetio::EntityReference expected1{"expectedRef1"};
      const openassetio::EntityReference expected2{"expectedRef2"};
      const openassetio::EntityReference expected3{"expectedRef3"};
      std::vector expectedVec{expected1, expected2, expected3};

      // With success callback side effect
      REQUIRE_CALL(mockManagerInterface, register_(refs, threeTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(0, expectedVec[0]))
          .LR_SIDE_EFFECT(_6(1, expectedVec[1]))
          .LR_SIDE_EFFECT(_6(2, expectedVec[2]));

      WHEN("batch register is called with default errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->register_(refs, threeTraitsDatas, kPublishingAccess, context);
        THEN("returned list of EntityReferences is as expected") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch register is called with kException errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->register_(refs, threeTraitsDatas, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned list of EntityReferences is as expected") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch register is called with kVariant errorPolicyTag") {
        std::vector<
            std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference>>
            actualVec = manager->register_(refs, threeTraitsDatas, kPublishingAccess, context,
                                           hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants contains the expected EntityReferences") {
          CHECK(expectedVec.size() == actualVec.size());
          for (size_t i = 0; i < actualVec.size(); ++i) {
            CHECK(std::holds_alternative<openassetio::EntityReference>(actualVec[i]));
            auto actualVal = std::get<openassetio::EntityReference>(actualVec[i]);
            CHECK(expectedVec[i] == actualVal);
          }
        }
      }
    }
    GIVEN(
        "manager plugin successfully registers multiple entity references in a non-index order") {
      const openassetio::EntityReferences refs = {openassetio::EntityReference{"ref1"},
                                                  openassetio::EntityReference{"ref2"},
                                                  openassetio::EntityReference{"ref3"}};

      const openassetio::EntityReference expected1{"expectedRef1"};
      const openassetio::EntityReference expected2{"expectedRef2"};
      const openassetio::EntityReference expected3{"expectedRef3"};
      std::vector expectedVec{expected1, expected2, expected3};

      // With success callback side effect, given out of order.
      REQUIRE_CALL(mockManagerInterface, register_(refs, threeTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(2, expectedVec[2]))
          .LR_SIDE_EFFECT(_6(0, expectedVec[0]))
          .LR_SIDE_EFFECT(_6(1, expectedVec[1]));

      WHEN("batch register is called with default errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->register_(refs, threeTraitsDatas, kPublishingAccess, context);
        THEN("returned list of EntityReferences is ordered in index order") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch register is called with kException errorPolicyTag") {
        const std::vector<openassetio::EntityReference> actualVec =
            manager->register_(refs, threeTraitsDatas, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kException);
        THEN("returned list of EntityReferences is ordered in index order") {
          CHECK(expectedVec == actualVec);
        }
      }
      WHEN("batch register is called with kVariant errorPolicyTag") {
        std::vector<
            std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference>>
            actualVec = manager->register_(refs, threeTraitsDatas, kPublishingAccess, context,
                                           hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants is ordered in index order") {
          CHECK(expectedVec.size() == actualVec.size());
          for (size_t i = 0; i < actualVec.size(); ++i) {
            CHECK(std::holds_alternative<openassetio::EntityReference>(actualVec[i]));
            auto actualVal = std::get<openassetio::EntityReference>(actualVec[i]);
            CHECK(expectedVec[i] == actualVal);
          }
        }
      }
    }
    GIVEN(
        "manager plugin will encounter an entity-specific error when next registering an "
        "EntityReference") {
      const openassetio::EntityReference ref{"testReference"};
      const openassetio::EntityReferences refs = {ref};

      const openassetio::errors::BatchElementError expected{ErrorCode::kMalformedEntityReference,
                                                            "Error Message"};

      // With error callback side effect
      REQUIRE_CALL(mockManagerInterface, register_(refs, singleTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_7(0, expected));

      WHEN("singular register is called with default errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->register_(ref, singleTraitsData, kPublishingAccess, context),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expected));
        }
      }
      WHEN("singular register is called with kException errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->register_(ref, singleTraitsData, kPublishingAccess, context,
                                 hostApi::Manager::BatchElementErrorPolicyTag::kException),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expected));
        }
      }
      WHEN("singular register is called with kVariant errorPolicyTag") {
        std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference> actual =
            manager->register_(ref, singleTraitsData, kPublishingAccess, context,
                               hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned variant contains the expected BatchElementError") {
          CHECK(std::holds_alternative<openassetio::errors::BatchElementError>(actual));
          const auto& actualVal = std::get<openassetio::errors::BatchElementError>(actual);
          CHECK(expected == actualVal);
        }
      }
    }
    GIVEN(
        "manager plugin will encounter entity-specific errors when next registering multiple "
        "EntityReferences") {
      const openassetio::EntityReferences refs = {openassetio::EntityReference{"ref1"},
                                                  openassetio::EntityReference{"ref2"},
                                                  openassetio::EntityReference{"ref3"}};

      const openassetio::EntityReference expectedValue2{"expectedRef2"};
      const openassetio::errors::BatchElementError expectedError0{
          ErrorCode::kMalformedEntityReference, "Malformed Mock Error🤖"};
      const openassetio::errors::BatchElementError expectedError1{ErrorCode::kEntityAccessError,
                                                                  "Entity Access Error Message"};

      // With mixed callback side effect
      REQUIRE_CALL(mockManagerInterface, register_(refs, threeTraitsDatas, kPublishingAccess,
                                                   context, hostSession, _, _))
          .LR_SIDE_EFFECT(_6(2, expectedValue2))
          .LR_SIDE_EFFECT(_7(0, expectedError0))
          .LR_SIDE_EFFECT(_7(1, expectedError1));

      WHEN("batch register is called with default errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->register_(refs, threeTraitsDatas, kPublishingAccess, context),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expectedError0));
        }
      }
      WHEN("batch register is called with kException errorPolicyTag") {
        THEN("an exception is thrown") {
          CHECK_THROWS_MATCHES(
              manager->register_(refs, threeTraitsDatas, kPublishingAccess, context,
                                 hostApi::Manager::BatchElementErrorPolicyTag::kException),
              openassetio::errors::BatchElementException,
              makeErrorExceptionMatchPredicate(expectedError0));
        }
      }
      WHEN("batch register is called with kVariant errorPolicyTag") {
        std::vector<
            std::variant<openassetio::errors::BatchElementError, openassetio::EntityReference>>
            actualVec = manager->register_(refs, threeTraitsDatas, kPublishingAccess, context,
                                           hostApi::Manager::BatchElementErrorPolicyTag::kVariant);
        THEN("returned lists of variants contains the expected objects") {
          auto error0 = std::get<openassetio::errors::BatchElementError>(actualVec[0]);
          CHECK(error0 == expectedError0);

          auto error1 = std::get<openassetio::errors::BatchElementError>(actualVec[1]);
          CHECK(error1 == expectedError1);

          CHECK(std::get<openassetio::EntityReference>(actualVec[2]) == expectedValue2);
        }
      }
    }
  }
}
