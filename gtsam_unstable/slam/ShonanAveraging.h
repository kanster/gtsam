/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010-2019, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file   ShonanAveraging.h
 * @date   March 2019
 * @author Frank Dellaert
 * @brief  Shonan Averaging algorithm
 */

#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/SO3.h>
#include <gtsam/nonlinear/LevenbergMarquardtParams.h>
#include <gtsam/slam/dataset.h>
#include <gtsam/base/Matrix.h>

#include <Eigen/Sparse>

#include <map>
#include <string>

namespace gtsam {
class NonlinearFactorGraph;

/// Parameters governing optimization etc.
struct ShonanAveragingParameters {
  bool prior;                   // whether to use a prior (default true)
  bool karcher;                 // whether to use Karcher mean prior (default true)
  double noiseSigma;            // Optional noise Sigma, will be ignored if zero
  double optimalityThreshold;   // threshold used in checkOptimality
  LevenbergMarquardtParams lm;  // LM parameters
  ShonanAveragingParameters(const std::string& verbosity = "SILENT",
                            const std::string& method = "JACOBI",
                            double noiseSigma = 0,
                            double optimalityThreshold = -1e-4);
  void setPrior(bool value) { prior = value; }
  void setKarcher(bool value) { karcher = value; }
  void setNoiseSigma(bool value) { noiseSigma = value; }
};

class ShonanAveraging {
 public:
  using Sparse = Eigen::SparseMatrix<double>;

 private:
  ShonanAveragingParameters parameters_;
  BetweenFactorPose3s factors_;
  std::map<Key, Pose3> poses_;
  size_t d_;  // dimensionality (typically 2 or 3)
  Sparse D_;  // Sparse (diagonal) degree matrix
  Sparse Q_;  // Sparse measurement matrix, == \tilde{R} in Eriksson18cvpr
  Sparse L_;  // connection Laplacian L = D - Q, needed for optimality check

  /**
   * Build 3Nx3N sparse matrix consisting of rotation measurements, arranged as
   *       (i,j) and (j,i) blocks within a sparse matrix.
   * @param useNoiseModel whether to use noise model
   */
  Sparse buildQ(bool useNoiseModel = false) const;

  /**
   * Build 3Nx3N sparse degree matrix D
   * @param useNoiseModel whether to use noise model
   */
  Sparse buildD(bool useNoiseModel = false) const;

 public:
  /**
   * Construct from a G2O file
   */
  explicit ShonanAveraging(const std::string& g2oFile,
                           const ShonanAveragingParameters& parameters =
                               ShonanAveragingParameters());

  /// Return number of poses
  size_t nrPoses() const { return poses_.size(); }

  /// k^th measurement, as a Pose3.
  const Pose3& measured(size_t i) const { return factors_[i]->measured(); }

  /// Keys for k^th measurement, as a vector of Key values.
  const KeyVector& keys(size_t i) const { return factors_[i]->keys(); }

  /// Return poses
  const std::map<Key, Pose3>& poses() const { return poses_; }
    
  Sparse D() const {return D_;} ///< Sparse version of D
  Matrix denseD() const {return Matrix(D_);} ///< Dense version of D
  Sparse Q() const {return Q_;} ///< Sparse version of Q
  Matrix denseQ() const {return Matrix(Q_);} ///< Dense version of Q
  Sparse L() const {return L_;} ///< Sparse version of L
  Matrix denseL() const {return Matrix(L_);} ///< Dense version of L

  /**
   * Build graph for SO(p)
   * @param p the dimensionality of the rotation manifold to optimize over
   */
  NonlinearFactorGraph buildGraphAt(size_t p) const;

  /**
   * Initialize randomly at SO(p)
   * @param p the dimensionality of the rotation manifold to optimize over
   */
  Values initializeRandomlyAt(size_t p) const;

  /**
   * Calculate cost for SO(p)
   * Values should be of type SO(p)
   */
  double costAt(size_t p, const Values& values) const;

  /**
   * Given an estimated local minimum Yopt for the (possibly lifted)
   * relaxation, this function computes and returns the block-diagonal elements
   * of the corresponding Lagrange multiplier.
   */
  Sparse computeLambda(const Values& values) const;

  /// Version that takes pxdN Stiefel manifold elements
  Sparse computeLambda(const Matrix& S) const;

  /// Dense versions of computeLambda for wrapper/testing
  Matrix computeLambda_(const Values& values) const {
    return Matrix(computeLambda(values));
  }
  Matrix computeLambda_(const Matrix& S) const {
    return Matrix(computeLambda(S));
  }

  /// Compute A matrix whose Eigenvalues we will examine
  Sparse computeA(const Values& values) const;

  /// Version that takes pxdN Stiefel manifold elements
  Sparse computeA(const Matrix& S) const;

  /// Dense version of computeA for wrapper/testing
  Matrix computeA_(const Values& values) const {
    return Matrix(computeA(values));
  }

  /** 
   * Compute minimum eigenvalue for optimality check.
   * @param values: should be of type SOn
   */
  double computeMinEigenValue(const Values& values, Vector* minEigenVector = nullptr) const;

  /**
   * Check optimality
   * @param values: should be of type SOn
   */
  bool checkOptimality(const Values& values) const;

  /**
   * Try to optimize at SO(p)
   * @param p the dimensionality of the rotation manifold to optimize over
   * @param initial optional initial SO(p) values
   * @return SO(p) values
   */
  Values tryOptimizingAt(
      size_t p,
      const boost::optional<const Values&> initial = boost::none) const;

  /**
   * Project from SO(p) to SO(3)
   * Values should be of type SO(p)
   */
  Values projectFrom(size_t p, const Values& values) const;

  /**
   * Project pxdN Stiefel manifold matrix S to SO(3)^N
   */
  Values roundSolution(const Matrix S) const;
  
  /**
   * Project from SO(p)^N to SO(3)^N
   * Values should be of type SO(p)
   */
  Values roundSolution(const Values& values) const;

  /**
   * Calculate cost for SO(3)
   * Values should be of type SO3
   */
  double cost(const Values& values) const;

  /// Create a tangent direction xi with eigenvector segment v_i
  static Vector MakeATangentVector(size_t p, const Vector& v, size_t i);

  /**
   * Calculate the riemannian gradient of F(values) at values 
   */
  Matrix riemannianGradient(size_t p, const Values& values) const;

  /**
   *  Lift up the dimension of values in type SO(p-1) with descent direction provided by minEigenVector
   * and return new values in type SO(p)
   */
  Values dimensionLifting(
    size_t p, const Values& values, const Vector& minEigenVector) const;

  /**
   * Given some values at p-1, return new values at p, by doing a line search
   * along the descent direction, computed from the minimum eigenvector at p-1.
   * @param values should be of type SO(p-1)
   * @param minEigenVector corresponding to minEigenValue at level p-1
   * @return values of type SO(p)
   */
  Values initializeWithDescent(size_t p, const Values& values,
                               const Vector& minEigenVector, double minEigenValue, double gradienTolerance=1e-2, double preconditionedGradNormTolerance=1e-4) const;

  /**
   * Optimize at different values of p until convergence, with random init at each level.
   * @param pMin value of p to start Riemanian staircase at.
   * @param pMax maximum value of p to try (default: 20)
   * @param withDescent use descent direction from paper.
   * @return (SO(3) values, minimum eigenvalue)
   */
  std::pair<Values, double> run(size_t pMin, size_t pMax, bool withDescent) const;

  /**
   * Optimize at different values of p until convergence, with random init at each level.
   */
  std::pair<Values, double> runWithRandom(size_t pMin = 5, size_t pMax = 20) const;

  /**
   * Optimize at different values of p until convergence, with descent direction.
   */
  std::pair<Values, double> runWithDescent(size_t pMin = 5, size_t pMax = 20) const;
};

}  // namespace gtsam