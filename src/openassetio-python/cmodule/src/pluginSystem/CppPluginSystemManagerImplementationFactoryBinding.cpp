// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 The Foundry Visionmongers Ltd
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <openassetio/log/LoggerInterface.hpp>
#include <openassetio/managerApi/ManagerInterface.hpp>
#include <openassetio/pluginSystem/CppPluginSystemManagerImplementationFactory.hpp>

#include "../_openassetio.hpp"
#include "../overrideMacros.hpp"

void registerCppPluginSystemManagerImplementationFactory(const py::module_& mod) {
  using openassetio::hostApi::ManagerImplementationFactoryInterface;
  using openassetio::log::LoggerInterfacePtr;
  using openassetio::pluginSystem::CppPluginSystemManagerImplementationFactory;

  py::class_<CppPluginSystemManagerImplementationFactory, ManagerImplementationFactoryInterface,

             CppPluginSystemManagerImplementationFactory::Ptr>(
      mod, "CppPluginSystemManagerImplementationFactory", py::is_final())
      .def_readonly_static("kPluginEnvVar",
                           &CppPluginSystemManagerImplementationFactory::kPluginEnvVar)
      .def(py::init(
               RetainCommonPyArgs::forFn<py::overload_cast<openassetio::Str, LoggerInterfacePtr>(
                   &CppPluginSystemManagerImplementationFactory::make)>()),
           py::arg("paths"), py::arg("logger").none(false))
      .def(py::init(RetainCommonPyArgs::forFn<py::overload_cast<LoggerInterfacePtr>(
                        &CppPluginSystemManagerImplementationFactory::make)>()),
           py::arg("logger").none(false))
      .def("identifiers", &CppPluginSystemManagerImplementationFactory::identifiers,
           py::call_guard<py::gil_scoped_release>{})
      .def("instantiate", &CppPluginSystemManagerImplementationFactory::instantiate,
           py::arg("identifier"), py::call_guard<py::gil_scoped_release>{});
}
