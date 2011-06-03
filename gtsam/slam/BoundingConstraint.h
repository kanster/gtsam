/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file BoundingConstraint.h
 * @brief Provides partially implemented constraints to implement bounds
 * @author Alex Cunningham
 */

#pragma once

#include <gtsam/base/Lie.h>
#include <gtsam/nonlinear/NonlinearConstraint.h>

namespace gtsam {

/**
 * Unary inequality constraint forcing a scalar to be
 * greater/less than a fixed threshold.  The function
 * will need to have its value function implemented to return
 * a scalar for comparison.
 */
template<class VALUES, class KEY>
struct BoundingConstraint1: public NonlinearConstraint1<VALUES, KEY> {
	typedef typename KEY::Value X;
	typedef NonlinearConstraint1<VALUES, KEY> Base;
	typedef boost::shared_ptr<BoundingConstraint1<VALUES, KEY> > shared_ptr;

	double threshold_;
	bool isGreaterThan_; /// flag for greater/less than

	BoundingConstraint1(const KEY& key, double threshold,
			bool isGreaterThan, double mu = 1000.0) :
				Base(key, 1, mu), threshold_(threshold), isGreaterThan_(isGreaterThan) {
	}

	inline double threshold() const { return threshold_; }
	inline bool isGreaterThan() const { return isGreaterThan_; }

	/**
	 * function producing a scalar value to compare to the threshold
	 * Must have optional argument for derivative with 1xN matrix, where
	 * N = X::dim()
	 */
	virtual double value(const X& x, boost::optional<Matrix&> H =
			boost::none) const = 0;

	/** active when constraint *NOT* met */
	bool active(const VALUES& c) const {
		// note: still active at equality to avoid zigzagging
		double x = value(c[this->key_]);
		return (isGreaterThan_) ? x <= threshold_ : x >= threshold_;
	}

	Vector evaluateError(const X& x, boost::optional<Matrix&> H =
			boost::none) const {
		Matrix D;
		double error = value(x, D) - threshold_;
		if (H) {
			if (isGreaterThan_) *H = D;
			else *H = -1.0 * D;
		}

		if (isGreaterThan_)
			return Vector_(1, error);
		else
			return -1.0 * Vector_(1, error);
	}

private:

	/** Serialization function */
	friend class boost::serialization::access;
	template<class ARCHIVE>
	void serialize(ARCHIVE & ar, const unsigned int version) {
		ar & boost::serialization::make_nvp("NonlinearConstraint1",
				boost::serialization::base_object<Base>(*this));
		ar & BOOST_SERIALIZATION_NVP(threshold_);
		ar & BOOST_SERIALIZATION_NVP(isGreaterThan_);
	}
};

/**
 * Binary scalar inequality constraint, with a similar value() function
 * to implement for specific systems
 */
template<class VALUES, class KEY1, class KEY2>
struct BoundingConstraint2: public NonlinearConstraint2<VALUES, KEY1, KEY2> {
	typedef typename KEY1::Value X1;
	typedef typename KEY2::Value X2;

	typedef NonlinearConstraint2<VALUES, KEY1, KEY2> Base;
	typedef boost::shared_ptr<BoundingConstraint2<VALUES, KEY1, KEY2> > shared_ptr;

	double threshold_;
	bool isGreaterThan_; /// flag for greater/less than

	BoundingConstraint2(const KEY1& key1, const KEY2& key2, double threshold,
			bool isGreaterThan, double mu = 1000.0)
	: Base(key1, key2, 1, mu), threshold_(threshold), isGreaterThan_(isGreaterThan) {}

	inline double threshold() const { return threshold_; }
	inline bool isGreaterThan() const { return isGreaterThan_; }

	/**
	 * function producing a scalar value to compare to the threshold
	 * Must have optional argument for derivatives)
	 */
	virtual double value(const X1& x1, const X2& x2,
			boost::optional<Matrix&> H1 = boost::none,
			boost::optional<Matrix&> H2 = boost::none) const = 0;

	/** active when constraint *NOT* met */
	bool active(const VALUES& c) const {
		// note: still active at equality to avoid zigzagging
		double x = value(c[this->key1_], c[this->key2_]);
		return (isGreaterThan_) ? x <= threshold_ : x >= threshold_;
	}

	Vector evaluateError(const X1& x1, const X2& x2,
			boost::optional<Matrix&> H1 = boost::none,
			boost::optional<Matrix&> H2 = boost::none) const {
		Matrix D1, D2;
		double error = value(x1, x2, D1, D2) - threshold_;
		if (H1) {
			if (isGreaterThan_)	*H1 = D1;
			else *H1 = -1.0 * D1;
		}
		if (H2) {
			if (isGreaterThan_) *H2 = D2;
			else *H2 = -1.0 * D2;
		}

		if (isGreaterThan_)
			return Vector_(1, error);
		else
			return -1.0 * Vector_(1, error);
	}

private:

	/** Serialization function */
	friend class boost::serialization::access;
	template<class ARCHIVE>
	void serialize(ARCHIVE & ar, const unsigned int version) {
		ar & boost::serialization::make_nvp("NonlinearConstraint2",
				boost::serialization::base_object<Base>(*this));
		ar & BOOST_SERIALIZATION_NVP(threshold_);
		ar & BOOST_SERIALIZATION_NVP(isGreaterThan_);
	}
};

} // \namespace gtsam
