/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file   Cal3_S2Stereo.h
 * @brief  The most common 5DOF 3D->2D calibration + Stereo baseline
 * @author Chris Beall
 */

#pragma once

#include <gtsam/geometry/Cal3_S2.h>
#include <iosfwd>

namespace gtsam {

  /**
   * @brief The most common 5DOF 3D->2D calibration, stereo version
   * @addtogroup geometry
   * \nosubgrouping
   */
  class GTSAM_EXPORT Cal3_S2Stereo : public Cal3_S2 {
  private:
    double b_;

  public:

    enum { dimension = 6 };
    ///< shared pointer to stereo calibration object
    typedef boost::shared_ptr<Cal3_S2Stereo> shared_ptr;

    /// @name Standard Constructors
    /// @

    /// default calibration leaves coordinates unchanged
    Cal3_S2Stereo() : Cal3_S2(1, 1, 0, 0, 0), b_(1.0) {}

    /// constructor from doubles
    Cal3_S2Stereo(double fx, double fy, double s, double u0, double v0,
                  double b)
        : Cal3_S2(fx, fy, s, u0, v0), b_(b) {}

    /// constructor from vector
    Cal3_S2Stereo(const Vector& d)
        : Cal3_S2(d(0), d(1), d(2), d(3), d(4)), b_(d(5)) {}

    /// easy constructor; field-of-view in degrees, assumes zero skew
    Cal3_S2Stereo(double fov, int w, int h, double b)
        : Cal3_S2(fov, w, h), b_(b) {}

    /// @}
    /// @name Testable
    /// @{

    void print(const std::string& s = "") const override;

    /// Check if equal up to specified tolerance
    bool equals(const Cal3_S2Stereo& other, double tol = 10e-9) const;

   /// @}
    /// @name Standard Interface
    /// @{

    /// return calibration, same for left and right
    const Cal3_S2& calibration() const { return *this; }

    /// return calibration matrix K, same for left and right
    Matrix3 K() const { return K(); }

    /// return baseline
    inline double baseline() const { return b_; }

    /// vectorized form (column-wise)
    Vector6 vector() const {
      Vector6 v;
      v << vector(), b_;
      return v;
    }

    /// @}
    /// @name Manifold
    /// @{

    /// return DOF, dimensionality of tangent space
    inline size_t dim() const { return dimension; }

    /// return DOF, dimensionality of tangent space
    static size_t Dim() { return dimension; }

    /// Given 6-dim tangent vector, create new calibration
    inline Cal3_S2Stereo retract(const Vector& d) const {
      return Cal3_S2Stereo(fx() + d(0), fy() + d(1), skew() + d(2), px() + d(3),
                           py() + d(4), b_ + d(5));
    }

    /// Unretraction for the calibration
    Vector6 localCoordinates(const Cal3_S2Stereo& T2) const {
      return T2.vector() - vector();
    }


    /// @}
    /// @name Advanced Interface
    /// @{

  private:
    /** Serialization function */
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::make_nvp(
          "Cal3_S2", boost::serialization::base_object<Cal3_S2>(*this));
      ar& BOOST_SERIALIZATION_NVP(b_);
    }
    /// @}

  };

  // Define GTSAM traits
  template<>
  struct traits<Cal3_S2Stereo> : public internal::Manifold<Cal3_S2Stereo> {
  };

  template<>
  struct traits<const Cal3_S2Stereo> : public internal::Manifold<Cal3_S2Stereo> {
  };

} // \ namespace gtsam
