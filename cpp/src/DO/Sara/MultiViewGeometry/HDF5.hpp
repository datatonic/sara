// ========================================================================== //
// This file is part of Sara, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2019 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

#pragma once

#include <DO/Sara/Core/HDF5.hpp>
#include <DO/Sara/MultiViewGeometry/EpipolarGraph.hpp>
#include <DO/Sara/MultiViewGeometry/Geometry/FundamentalMatrix.hpp>
#include <DO/Sara/MultiViewGeometry/Geometry/PinholeCamera.hpp>


namespace DO::Sara {

  //! @addtogroup MultiViewGeometry
  //! @{

  template <>
  struct CalculateH5Type<EssentialMatrix>
  {
    static inline auto value()
    {
      return CalculateH5Type<Matrix3d>::value();
    }
  };

  template <>
  struct CalculateH5Type<FundamentalMatrix>
  {
    static inline auto value()
    {
      return CalculateH5Type<Matrix3d>::value();
    }
  };

  template <>
  struct CalculateH5Type<PinholeCamera>
  {
    static inline auto value() -> H5::CompType
    {
      auto h5_comp_type = H5::CompType{sizeof(PinholeCamera)};
      INSERT_MEMBER(h5_comp_type, PinholeCamera, K);
      INSERT_MEMBER(h5_comp_type, PinholeCamera, R);
      INSERT_MEMBER(h5_comp_type, PinholeCamera, t);
      return h5_comp_type;
    }
  };

  //! @}

} /* namespace DO::Sara */
