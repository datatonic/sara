// ========================================================================== //
// This file is part of DO-CV, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2013 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

#include <Eigen/StdVector>

#include <DO/Sara/FeatureDescriptors.hpp>


using namespace std;


namespace DO { namespace Sara {

  ComputeDominantOrientations::
  ComputeDominantOrientations(float peak_ratio_thres,
                              float patchTruncationFactor,
                              float blurFactor)
    : _peak_ratio_thres(peak_ratio_thres)
    , _patch_truncation_factor(patchTruncationFactor)
    , _blur_factor(blurFactor) {}

  std::vector<float>
  ComputeDominantOrientations::
  operator()(const Image<Vector2f>& gradients,
             float x, float y, float sigma) const
  {
    // Compute histogram of gradients as in [Lowe, IJCV 2004].
    Array<float, 36, 1> orientation_histogram;
    compute_orientation_histogram(orientation_histogram, gradients, x, y, sigma,
                                _patch_truncation_factor, _blur_factor );

    // Smooth as in [Lowe, IJCV 2004].
    lowe_smooth_histogram(orientation_histogram);
    vector<int> peaksInt(find_peaks(orientation_histogram, _peak_ratio_thres));

    // Refine peaks as in [Lowe, IJCV 2004].
    vector<float> peaks_(refine_peaks(orientation_histogram, peaksInt));

    // Convert orientation to radian.
    for (size_t i = 0; i != peaks_.size(); ++i)
    {
#define LOWE
#ifndef LOWE
      // Orientations in $[0, 2\pi[$
      peaks_[i] *= float(M_PI)/36.f;
      // Orientations in $[-\pi, \pi[$
      if (peaks_[i] > float(M_PI))
        peaks_[i] -= 2.f*float(M_PI);
#else
      peaks_[i] = 2.0f * float(M_PI) * peaks_[i] / 36.f - float(M_PI);
#endif
    }

    return peaks_;
  }

  std::vector<float>
  ComputeDominantOrientations::
  operator()(const ImagePyramid<Vector2f>& pyramid,
             const OERegion& extremum,
             const Point2i& scaleOctPair) const
  {
    float x = extremum.x();
    float y = extremum.y();
    int s = scaleOctPair(0);
    int o = scaleOctPair(1);
    return this->operator()(pyramid(s,o), x, y, s);
  }

  void
  ComputeDominantOrientations::
  operator()(const ImagePyramid<Vector2f>& pyramid,
             vector<OERegion>& extrema,
             vector<Point2i>& scaleOctPairs) const
  {
    using namespace std;
    vector<OERegion> e2;
    vector<Point2i> so2;
    e2.reserve(extrema.size()*2);
    so2.reserve(extrema.size()*2);
    for (size_t i = 0; i != extrema.size(); ++i)
    {
      vector<float> ori;
      ori = this->operator()(pyramid, extrema[i], scaleOctPairs[i]);
      for (size_t o = 0; o != ori.size(); ++o)
      {
        // Recopy.
        so2.push_back(scaleOctPairs[i]);
        e2.push_back(extrema[i]);
        // Assign dominant orientation.
        e2.back().orientation() = ori[o];
      }
    }

    e2.swap(extrema);
    so2.swap(scaleOctPairs);
  }

} /* namespace Sara */
} /* namespace DO */
