// SPDX-License-Identifier: Apache-2.0
// Copyright 2013-2025 The Foundry Visionmongers Ltd
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <openassetio/export.h>
#include <openassetio/managerApi/EntityReferencePagerInterface.hpp>
#include <openassetio/managerApi/HostSession.hpp>

#include "../_openassetio.hpp"
#include "../overrideMacros.hpp"

namespace openassetio {
inline namespace OPENASSETIO_CORE_ABI_VERSION {
namespace managerApi {

/**
 * Trampoline class required for pybind to bind pure virtual methods
 * and allow C++ -> Python calls via a C++ instance.
 */
struct PyEntityReferencePagerInterface : EntityReferencePagerInterface {
  using EntityReferencePagerInterface::EntityReferencePagerInterface;

  bool hasNext(const HostSessionPtr& hostSession) override {
    OPENASSETIO_PYBIND11_OVERRIDE_PURE(bool, EntityReferencePagerInterface, hasNext, hostSession);
  }

  EntityReferencePagerInterface::Page get(const HostSessionPtr& hostSession) override {
    OPENASSETIO_PYBIND11_OVERRIDE_PURE(EntityReferencePagerInterface::Page,
                                       EntityReferencePagerInterface, get, hostSession);
  }

  void next(const HostSessionPtr& hostSession) override {
    OPENASSETIO_PYBIND11_OVERRIDE_PURE(void, EntityReferencePagerInterface, next, hostSession);
  }

  void close(const HostSessionPtr& hostSession) override {
    OPENASSETIO_PYBIND11_OVERRIDE(void, EntityReferencePagerInterface, close, hostSession);
  }
};

}  // namespace managerApi
}  // namespace OPENASSETIO_CORE_ABI_VERSION
}  // namespace openassetio

void registerEntityReferencePagerInterface(const py::module& mod) {
  using openassetio::managerApi::EntityReferencePagerInterface;
  using openassetio::managerApi::EntityReferencePagerInterfacePtr;
  using openassetio::managerApi::PyEntityReferencePagerInterface;

  py::class_<EntityReferencePagerInterface, PyEntityReferencePagerInterface,
             EntityReferencePagerInterfacePtr>(mod, "EntityReferencePagerInterface")
      .def(py::init())
      .def("hasNext", &EntityReferencePagerInterface::hasNext, py::arg("hostSession").none(false),
           py::call_guard<py::gil_scoped_release>{})
      .def("get", &EntityReferencePagerInterface::get, py::arg("hostSession").none(false),
           py::call_guard<py::gil_scoped_release>{})
      .def("next", &EntityReferencePagerInterface::next, py::arg("hostSession").none(false),
           py::call_guard<py::gil_scoped_release>{})
      .def("close", &EntityReferencePagerInterface::close, py::arg("hostSession").none(false),
           py::call_guard<py::gil_scoped_release>{});
}
