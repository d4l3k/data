/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <f4d/common/memory/Memory.h>
#include <f4d/type/Type.h>
#include <f4d/vector/TypeAliases.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <iostream>
#include "f4d/functions/common/CoreFunctions.h"
#include "column.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

using namespace facebook::f4d;
using namespace facebook::torcharrow;

PYBIND11_MAKE_OPAQUE(std::vector<bool>);

template <
    TypeKind kind,
    typename D,
    typename T = typename TypeTraits<kind>::NativeType>
py::class_<SimpleColumn<T>, BaseColumn> declareSimpleType(
    py::module& m,
    const D& decoder) {
  py::class_<SimpleColumn<T>, BaseColumn> result(
      m, (std::string("SimpleColumn") + TypeTraits<kind>::name).c_str());
  result
      .def(
          "__getitem__",
          [&decoder](SimpleColumn<T>& self, int index) {
            return decoder(self.valueAt(index));
          })
      .def("append_null", &SimpleColumn<T>::appendNull)
      .def("slice", &SimpleColumn<T>::slice);

  py::class_<FlatColumn<T>, SimpleColumn<T>>(
      m, (std::string("FlatColumn") + TypeTraits<kind>::name).c_str());

  using I = typename TypeTraits<kind>::ImplType;
  py::class_<I, Type, std::shared_ptr<I>>(m, TypeTraits<kind>::name)
      .def(py::init());

  m.def("Column", [](std::shared_ptr<I> type) {
    return std::make_unique<SimpleColumn<T>>();
  });
  return result;
};

void declareArrayType(py::module& m) {
  py::class_<ArrayColumn, BaseColumn>(m, "ArrayColumn")
      .def("append", &ArrayColumn::appendElement)
      .def("append_null", &ArrayColumn::appendNull)
      .def("__getitem__", &ArrayColumn::valueAt)
      .def("slice", &ArrayColumn::slice);

  using I = typename TypeTraits<TypeKind::ARRAY>::ImplType;
  py::class_<I, Type, std::shared_ptr<I>>(m, TypeTraits<TypeKind::ARRAY>::name)
      .def(py::init<TypePtr>())
      .def("element_type", &ArrayType::elementType);
  m.def("Column", [](std::shared_ptr<I> type) {
    return std::make_unique<ArrayColumn>(type);
  });
}

void declareMapType(py::module& m) {
  py::class_<MapColumn, BaseColumn>(m, "MapColumn")
      .def("append", &MapColumn::appendElement)
      .def("append_null", &MapColumn::appendNull)
      .def("offset_at", &MapColumn::offsetAt)
      .def("size_at", &MapColumn::sizeAt)
      .def("__getitem__", &MapColumn::valueAt)
      .def("keys", &MapColumn::mapKeys)
      .def("values", &MapColumn::mapValues)
      .def("slice", &MapColumn::slice);

  using I = typename TypeTraits<TypeKind::MAP>::ImplType;
  py::class_<I, Type, std::shared_ptr<I>>(m, TypeTraits<TypeKind::MAP>::name)
      .def(py::init<TypePtr, TypePtr>());
  m.def("Column", [](std::shared_ptr<I> type) {
    return std::make_unique<MapColumn>(type);
  });
}

void declareRowType(py::module& m) {
  py::class_<RowColumn, BaseColumn>(m, "RowColumn")
      .def("child_at", &RowColumn::childAt)
      .def("set_child", &RowColumn::setChild)
      .def("children_size", &RowColumn::childrenSize)
      .def("slice", &RowColumn::slice)
      .def("set_length", &RowColumn::setLength)
      .def("set_null_at", &RowColumn::setNullAt)
      .def("copy", &RowColumn::copy);

  using I = typename TypeTraits<TypeKind::ROW>::ImplType;
  py::class_<I, Type, std::shared_ptr<I>>(m, TypeTraits<TypeKind::ROW>::name)
      .def(py::init<
           std::vector<std::string>&&,
           std::vector<std::shared_ptr<const Type>>&&>())
      .def("get_child_idx", &I::getChildIdx)
      .def("contains_child", &I::containsChild)
      .def("name_of", &I::nameOf)
      .def("child_at", &I::childAt);
  m.def("Column", [](std::shared_ptr<I> type) {
    return std::make_unique<RowColumn>(type);
  });
}

PYBIND11_MODULE(_torcharrow, m) {
  m.doc() = R"pbdoc(
        TorchArrow native code module
        -----------------------

        .. currentmodule:: torcharrow

        .. autosummary::
           :toctree: _generate

        TypeKind
    )pbdoc";

  py::class_<BaseColumn>(m, "BaseColumn")
      .def("type", &BaseColumn::type)
      .def("is_null_at", &BaseColumn::isNullAt)
      .def("get_null_count", &BaseColumn::getNullCount)
      .def_property_readonly("offset", &BaseColumn::getOffset)
      .def_property_readonly("length", &BaseColumn::getLength)
      .def("__len__", &BaseColumn::getLength);

  py::class_<Type, std::shared_ptr<Type>>(m, "Type");

  declareSimpleType<TypeKind::BIGINT>(m, [](auto val) { return py::cast(val); })
      .def(
          "append",
          [](SimpleColumn<int64_t>& self, py::int_ value) {
            self.append(py::cast<int64_t>(value));
          })
      .def("neg", &SimpleColumn<int64_t>::neg);

  declareSimpleType<TypeKind::INTEGER>(m, [](auto val) {
    return py::cast(val);
  }).def("append", [](SimpleColumn<int32_t>& self, py::int_ value) {
    self.append(py::cast<int32_t>(value));
  });

  declareSimpleType<TypeKind::SMALLINT>(m, [](auto val) {
    return py::cast(val);
  }).def("append", [](SimpleColumn<int16_t>& self, py::int_ value) {
    self.append(py::cast<int16_t>(value));
  });

  declareSimpleType<TypeKind::TINYINT>(m, [](auto val) {
    return py::cast(val);
  }).def("append", [](SimpleColumn<int8_t>& self, py::int_ value) {
    self.append(py::cast<int8_t>(value));
  });

  declareSimpleType<TypeKind::BOOLEAN>(
      m, [](auto val) { return py::cast(val); })
      .def(
          "append",
          [](SimpleColumn<bool>& self, py::bool_ value) {
            self.append(py::cast<bool>(value));
          })
      .def("append", [](SimpleColumn<bool>& self, py::int_ value) {
        self.append(py::cast<bool>(value));
      });

  declareSimpleType<TypeKind::REAL>(m, [](auto val) { return py::cast(val); })
      .def(
          "append",
          [](SimpleColumn<float>& self, py::float_ value) {
            self.append(py::cast<float>(value));
          })
      .def(
          "append",
          [](SimpleColumn<float>& self, py::int_ value) {
            self.append(py::cast<float>(value));
          })
      .def("neg", &SimpleColumn<float>::neg);

  declareSimpleType<TypeKind::DOUBLE>(m, [](auto val) { return py::cast(val); })
      .def(
          "append",
          [](SimpleColumn<double>& self, py::float_ value) {
            self.append(py::cast<double>(value));
          })
      .def(
          "append",
          [](SimpleColumn<double>& self, py::int_ value) {
            self.append(py::cast<double>(value));
          })
      .def("neg", &SimpleColumn<double>::neg);

  declareSimpleType<TypeKind::VARCHAR>(
      m,
      [](const auto& val) {
        return py::cast<py::str>(
            PyUnicode_DecodeUTF8(val.data(), val.size(), nullptr));
      })
      .def(
          "append",
          [](SimpleColumn<StringView>& self, const std::string& value) {
            self.append(StringView(value));
          });

  declareArrayType(m);
  declareMapType(m);
  declareRowType(m);

  py::register_exception<NotAppendableException>(m, "NotAppendableException");

  // Register Velox UDFs
  // TODO: we may only need to register UDFs that TorchArrow required?
  functions::registerFunctions();

#ifdef VERSION_INFO
      m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
      m.attr("__version__") = "dev";
#endif
}