// SPDX-License-Identifier: Apache-2.0
// Copyright 2013-2025 The Foundry Visionmongers Ltd
#include <cstddef>
#include <stdexcept>

#include <openassetio/c/StringView.h>
#include <openassetio/c/errors.h>

#include <catch2/catch.hpp>

#include <errors.hpp>

#include "StringViewReporting.hpp"
#include "openassetio/typedefs.hpp"

SCENARIO("throwIfError error code/message handling") {
  using openassetio::errors::throwIfError;

  GIVEN("an OK error code") {
    constexpr oa_ErrorCode kCode = oa_ErrorCode_kOK;

    WHEN("throwIfError is called") {
      THEN("no exception is thrown") { throwIfError(kCode, oa_StringView{}); }
    }
  }

  GIVEN("an error code and message") {
    const auto code = oa_ErrorCode_kUnknown;
    openassetio::Str message = "some error";

    const oa_StringView cmessage{
        message.size(),
        message.data(),
        message.size(),
    };

    WHEN("throwIfError is called") {
      THEN("expected exception is thrown") {
        REQUIRE_THROWS_MATCHES(throwIfError(code, cmessage), std::runtime_error,
                               Catch::Message("1: some error"));
      }
    }
  }
}

SCENARIO("Using extractExceptionMessage to copy a C++ exception message to a C StringView") {
  using openassetio::errors::extractExceptionMessage;

  GIVEN("An exception and a StringView") {
    const openassetio::Str expectedMessage = "some error";
    const std::runtime_error runtimeError{expectedMessage};

    openassetio::Str storage(expectedMessage.size(), '\0');
    oa_StringView actualMessage{storage.capacity(), storage.data(), 0};

    WHEN("extractExceptionMessage copies the message from the exception to the StringView") {
      extractExceptionMessage(&actualMessage, runtimeError);

      // Sanity check that `what()` always returns the same pointer on
      // this platform.
      CHECK(static_cast<const void*>(runtimeError.what()) ==
            static_cast<const void*>(runtimeError.what()));

      THEN("Message is copied into StringView") {
        CHECK(actualMessage == expectedMessage);
        CHECK(actualMessage.data != runtimeError.what());
      }
    }
  }
}

SCENARIO("Decorating an exception-throwing C++ function to instead return an error code") {
  using openassetio::errors::catchUnknownExceptionAsCode;

  // Error message storage.
  constexpr std::size_t kErrorStorageSize = 100;
  openassetio::Str storage(kErrorStorageSize, '\0');
  oa_StringView actualErrorMessage{storage.size(), storage.data(), 0};

  GIVEN("A callable that doesn't throw") {
    const auto callable = [&] { return oa_ErrorCode_kOK; };

    WHEN("callable is executed whilst decorated") {
      const oa_ErrorCode actualErrorCode =
          catchUnknownExceptionAsCode(&actualErrorMessage, callable);

      THEN("exception is caught and the error code and message is as expected") {
        CHECK(actualErrorCode == oa_ErrorCode_kOK);
        CHECK(actualErrorMessage == "");
      }
    }
  }

  GIVEN("A callable that throws an exception") {
    const openassetio::Str expectedErrorMessage = "some error";

    const auto callable = [&]() -> oa_ErrorCode { throw std::length_error{expectedErrorMessage}; };

    WHEN("callable is executed whilst decorated") {
      const oa_ErrorCode actualErrorCode =
          catchUnknownExceptionAsCode(&actualErrorMessage, callable);

      THEN("exception is caught and the error code and message is as expected") {
        CHECK(actualErrorCode == oa_ErrorCode_kException);
        CHECK(actualErrorMessage == expectedErrorMessage);
      }
    }
  }

  GIVEN("A callable that throws a non-exception") {
    const openassetio::Str expectedErrorMessage = "Unknown non-exception object thrown";

    const auto callable = [&]() -> oa_ErrorCode { throw "some error"; };

    WHEN("callable is executed whilst decorated") {
      const oa_ErrorCode actualErrorCode =
          catchUnknownExceptionAsCode(&actualErrorMessage, callable);

      THEN("exception is caught and the error code and message is as expected") {
        CHECK(actualErrorCode == oa_ErrorCode_kUnknown);
        CHECK(actualErrorMessage == expectedErrorMessage);
      }
    }
  }
}
