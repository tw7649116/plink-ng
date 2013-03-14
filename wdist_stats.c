#include "wdist_common.h"
#include "ipmpar.h"
#include "dcdflib.h"

double chiprob_p(double xx, double df) {
  int st = 0;
  int ww = 1;
  double bnd = 1;
  double pp;
  double qq;
  cdfchi(&ww, &pp, &qq, &xx, &df, &st, &bnd);
  if (st) {
    return -9;
  }
  return qq;
}

double inverse_chiprob(double qq, double df) {
  double pp = 1 - qq;
  int32_t st = 0;
  int32_t ww = 2;
  double bnd = 1;
  double xx;

  if (qq >= 1.0) {
    return 0;
  }
  cdfchi(&ww, &pp, &qq, &xx, &df, &st, &bnd);
  if (st != 0) {
    return -9;
  }
  return xx;
}

// Inverse normal distribution

//
// Lower tail quantile for standard normal distribution function.
//
// This function returns an approximation of the inverse cumulative
// standard normal distribution function.  I.e., given P, it returns
// an approximation to the X satisfying P = Pr{Z <= X} where Z is a
// random variable from the standard normal distribution.
//
// The algorithm uses a minimax approximation by rational functions
// and the result has a relative error whose absolute value is less
// than 1.15e-9.
//
// Author:      Peter J. Acklam
// Time-stamp:  2002-06-09 18:45:44 +0200
// E-mail:      jacklam@math.uio.no
// WWW URL:     http://www.math.uio.no/~jacklam
//
// C implementation adapted from Peter's Perl version

// Coefficients in rational approximations.

static const double ivn_a[] =
  {
    -3.969683028665376e+01,
    2.209460984245205e+02,
    -2.759285104469687e+02,
    1.383577518672690e+02,
    -3.066479806614716e+01,
     2.506628277459239e+00
  };

static const double ivn_b[] =
  {
    -5.447609879822406e+01,
    1.615858368580409e+02,
    -1.556989798598866e+02,
    6.680131188771972e+01,
    -1.328068155288572e+01
  };

static const double ivn_c[] =
  {
    -7.784894002430293e-03,
    -3.223964580411365e-01,
    -2.400758277161838e+00,
    -2.549732539343734e+00,
    4.374664141464968e+00,
     2.938163982698783e+00
  };

static const double ivn_d[] =
  {
    7.784695709041462e-03,
    3.224671290700398e-01,
    2.445134137142996e+00,
    3.754408661907416e+00
  };

#define IVN_LOW 0.02425
#define IVN_HIGH 0.97575

double ltqnorm(double p) {
  // assumes 0 < p < 1
  double q, r;

  if (p < IVN_LOW) {
    // Rational approximation for lower region
    q = sqrt(-2*log(p));
    return (((((ivn_c[0]*q+ivn_c[1])*q+ivn_c[2])*q+ivn_c[3])*q+ivn_c[4])*q+ivn_c[5]) /
      ((((ivn_d[0]*q+ivn_d[1])*q+ivn_d[2])*q+ivn_d[3])*q+1);
  } else if (p > IVN_HIGH) {
    // Rational approximation for upper region
    q  = sqrt(-2*log(1-p));
    return -(((((ivn_c[0]*q+ivn_c[1])*q+ivn_c[2])*q+ivn_c[3])*q+ivn_c[4])*q+ivn_c[5]) /
      ((((ivn_d[0]*q+ivn_d[1])*q+ivn_d[2])*q+ivn_d[3])*q+1);
  } else {
    // Rational approximation for central region
    q = p - 0.5;
    r = q*q;
    return (((((ivn_a[0]*r+ivn_a[1])*r+ivn_a[2])*r+ivn_a[3])*r+ivn_a[4])*r+ivn_a[5])*q /
      (((((ivn_b[0]*r+ivn_b[1])*r+ivn_b[2])*r+ivn_b[3])*r+ivn_b[4])*r+1);
  }
}

double fisher22(uint32_t m11, uint32_t m12, uint32_t m21, uint32_t m22) {
  // Basic 2x2 Fisher exact test p-value calculation.
  double tprob = (1 - SMALL_EPSILON) * EXACT_TEST_BIAS;
  double cur_prob = tprob;
  double cprob = 0;
  uint32_t uii;
  double cur11;
  double cur12;
  double cur21;
  double cur22;
  double preaddp;
  // Ensure we are left of the distribution center, m11 <= m22, and m12 <= m21.
  if (m12 > m21) {
    uii = m12;
    m12 = m21;
    m21 = uii;
  }
  if (m11 > m22) {
    uii = m11;
    m11 = m22;
    m22 = uii;
  }
  if ((((uint64_t)m11) * m22) > (((uint64_t)m12) * m21)) {
    uii = m11;
    m11 = m12;
    m12 = uii;
    uii = m21;
    m21 = m22;
    m22 = uii;
  }
  cur11 = m11;
  cur12 = m12;
  cur21 = m21;
  cur22 = m22;
  while (cur12 > 0.5) {
    cur11 += 1;
    cur22 += 1;
    cur_prob *= (cur12 * cur21) / (cur11 * cur22);
    cur12 -= 1;
    cur21 -= 1;
    if (cur_prob == INFINITY) {
      return 0;
    }
    if (cur_prob < EXACT_TEST_BIAS) {
      tprob += cur_prob;
      break;
    }
    cprob += cur_prob;
  }
  if (cprob == 0) {
    return 1;
  }
  if (cur12 > 0.5) {
    do {
      cur11 += 1;
      cur22 += 1;
      cur_prob *= (cur12 * cur21) / (cur11 * cur22);
      cur12 -= 1;
      cur21 -= 1;
      preaddp = tprob;
      tprob += cur_prob;
      if (tprob <= preaddp) {
	break;
      }
    } while (cur12 > 0.5);
  }
  if (m11) {
    cur11 = m11;
    cur12 = m12;
    cur21 = m21;
    cur22 = m22;
    cur_prob = (1 - SMALL_EPSILON) * EXACT_TEST_BIAS;
    do {
      cur12 += 1;
      cur21 += 1;
      cur_prob *= (cur11 * cur22) / (cur12 * cur21);
      cur11 -= 1;
      cur22 -= 1;
      preaddp = tprob;
      tprob += cur_prob;
      if (tprob <= preaddp) {
	return preaddp / (cprob + preaddp);
      }
    } while (cur11 > 0.5);
  }
  return tprob / (cprob + tprob);
}

void fisher22_precomp_thresh(uint32_t m11, uint32_t m12, uint32_t m21, uint32_t m22, uint32_t* m11_minp, uint32_t* m11_maxp, uint32_t* tiel, uint32_t* tieh) {
  // Treating m11 as the only variable, this returns the minimum and (maximum -
  // 1) values of m11 which are less extreme than the observed result.  If the
  // observed result is maximally common, the return values will both be zero.
  // Also reports which value(s) result in an identical p-value.
  double cur_prob = (1 - SMALL_EPSILON) * EXACT_TEST_BIAS;
  double cur11 = ((int32_t)m11);
  double cur12 = ((int32_t)m12);
  double cur21 = ((int32_t)m21);
  double cur22 = ((int32_t)m22);
  double ratio = (cur11 * cur22) / ((cur12 + 1) * (cur21 + 1));

  // Is m11 greater than the p-maximizing value?
  if (ratio > (1 + SMALL_EPSILON)) {
    *tieh = m11;
    *tiel = m11;
    *m11_maxp = m11;
    cur12 += 1;
    cur21 += 1;
    cur_prob *= ratio;
    do {
      cur11 -= 1;
      cur22 -= 1;
      m11--;
      cur12 += 1;
      cur21 += 1;
      cur_prob *= (cur11 * cur22) / (cur12 * cur21);
    } while (cur_prob > EXACT_TEST_BIAS);
    *m11_minp = m11;
    if (cur_prob > (1 - 2 * SMALL_EPSILON) * EXACT_TEST_BIAS) {
      *tiel = m11 - 1;
    }
  } else if (ratio > (1 - SMALL_EPSILON)) {
    *m11_minp = 0;
    *m11_maxp = 0;
    *tiel = m11 - 1;
    *tieh = m11;
  } else {
    // Is it less?
    *tiel = m11;
    cur11 += 1;
    cur22 += 1;
    ratio = (cur12 * cur21) / (cur11 * cur22);
    if (ratio > (1 + SMALL_EPSILON)) {
      *tieh = m11;
      cur_prob *= ratio;
      *m11_minp = ++m11;
      do {
	cur12 -= 1;
	cur21 -= 1;
	m11++;
	cur11 += 1;
	cur22 += 1;
	cur_prob *= (cur12 * cur21) / (cur11 * cur22);
      } while (cur_prob > EXACT_TEST_BIAS);
      *m11_maxp = m11;
      if (cur_prob > (1 - 2 * SMALL_EPSILON) * EXACT_TEST_BIAS) {
	*tieh = m11;
      }
    } else {
      *m11_minp = 0;
      *m11_maxp = 0;
      if (ratio > (1 - SMALL_EPSILON)) {
	*tieh = m11 + 1;
      }
    }
  }
}

int32_t fisher23_tailsum(double* base_probp, double* saved12p, double* saved13p, double* saved22p, double* saved23p, double *totalp, uint32_t right_side) {
  double total = 0;
  double cur_prob = *base_probp;
  double tmp12 = *saved12p;
  double tmp13 = *saved13p;
  double tmp22 = *saved22p;
  double tmp23 = *saved23p;
  double tmps12;
  double tmps13;
  double tmps22;
  double tmps23;
  double prev_prob;
  // identify beginning of tail
  if (right_side) {
    if (cur_prob > EXACT_TEST_BIAS) {
      prev_prob = tmp13 * tmp22;
      while (prev_prob > 0.5) {
	tmp12 += 1;
	tmp23 += 1;
	cur_prob *= prev_prob / (tmp12 * tmp23);
	tmp13 -= 1;
	tmp22 -= 1;
	if (cur_prob <= EXACT_TEST_BIAS) {
	  break;
	}
	prev_prob = tmp13 * tmp22;
      }
      *base_probp = cur_prob;
      tmps12 = tmp12;
      tmps13 = tmp13;
      tmps22 = tmp22;
      tmps23 = tmp23;
    } else {
      tmps12 = tmp12;
      tmps13 = tmp13;
      tmps22 = tmp22;
      tmps23 = tmp23;
      while (1) {
	prev_prob = cur_prob;
	tmp13 += 1;
	tmp22 += 1;
	cur_prob *= (tmp12 * tmp23) / (tmp13 * tmp22);
	if (cur_prob < prev_prob) {
	  return 1;
	}
	tmp12 -= 1;
	tmp23 -= 1;
	// throw in extra (1 - SMALL_EPSILON) multiplier to prevent rounding
	// errors from causing this to keep going when the left-side test
	// stopped
	if (cur_prob > (1 - SMALL_EPSILON) * EXACT_TEST_BIAS) {
	  break;
	}
	total += cur_prob;
      }
      prev_prob = cur_prob;
      cur_prob = *base_probp;
      *base_probp = prev_prob;
    }
  } else {
    if (cur_prob > EXACT_TEST_BIAS) {
      prev_prob = tmp12 * tmp23;
      while (prev_prob > 0.5) {
	tmp13 += 1;
	tmp22 += 1;
	cur_prob *= prev_prob / (tmp13 * tmp22);
	tmp12 -= 1;
	tmp23 -= 1;
	if (cur_prob <= EXACT_TEST_BIAS) {
	  break;
	}
	prev_prob = tmp12 * tmp23;
      }
      *base_probp = cur_prob;
      tmps12 = tmp12;
      tmps13 = tmp13;
      tmps22 = tmp22;
      tmps23 = tmp23;
    } else {
      tmps12 = tmp12;
      tmps13 = tmp13;
      tmps22 = tmp22;
      tmps23 = tmp23;
      while (1) {
	prev_prob = cur_prob;
	tmp12 += 1;
	tmp23 += 1;
	cur_prob *= (tmp13 * tmp22) / (tmp12 * tmp23);
	if (cur_prob < prev_prob) {
	  return 1;
	}
	tmp13 -= 1;
	tmp22 -= 1;
	if (cur_prob > EXACT_TEST_BIAS) {
	  break;
	}
	total += cur_prob;
      }
      prev_prob = cur_prob;
      cur_prob = *base_probp;
      *base_probp = prev_prob;
    }
  }
  *saved12p = tmp12;
  *saved13p = tmp13;
  *saved22p = tmp22;
  *saved23p = tmp23;
  if (cur_prob > EXACT_TEST_BIAS) {
    *totalp = 0;
    return 0;
  }
  // sum tail to floating point precision limit
  if (right_side) {
    prev_prob = total;
    total += cur_prob;
    while (total > prev_prob) {
      tmps12 += 1;
      tmps23 += 1;
      cur_prob *= (tmps13 * tmps22) / (tmps12 * tmps23);
      tmps13 -= 1;
      tmps22 -= 1;
      prev_prob = total;
      total += cur_prob;
    }
  } else {
    prev_prob = total;
    total += cur_prob;
    while (total > prev_prob) {
      tmps13 += 1;
      tmps22 += 1;
      cur_prob *= (tmps12 * tmps23) / (tmps13 * tmps22);
      tmps12 -= 1;
      tmps23 -= 1;
      prev_prob = total;
      total += cur_prob;
    }
  }
  *totalp = total;
  return 0;
}

double fisher23(uint32_t m11, uint32_t m12, uint32_t m13, uint32_t m21, uint32_t m22, uint32_t m23) {
  // 2x3 Fisher-Freeman-Halton exact test p-value calculation.
  // The number of tables involved here is still small enough that the network
  // algorithm (and the improved variants thereof that I've seen) are
  // suboptimal; a 2-dimensional version of the SNPHWE2 strategy has higher
  // performance.
  // 2x4, 2x5, and 3x3 should also be practical with this method, but beyond
  // that I doubt it's worth the trouble.
  // Complexity of approach is O(n^{df/2}), where n is number of observations.
  double cur_prob = (1 - SMALLISH_EPSILON) * EXACT_TEST_BIAS;
  double tprob = cur_prob;
  double cprob = 0;
  uint32_t dir = 0; // 0 = forwards, 1 = backwards
  double dyy = 0;
  double base_probl;
  double base_probr;
  double orig_base_probl;
  double orig_base_probr;
  double orig_row_prob;
  double row_prob;
  uint32_t uii;
  uint32_t ujj;
  uint32_t ukk;
  double cur11;
  double cur21;
  double savedl12;
  double savedl13;
  double savedl22;
  double savedl23;
  double savedr12;
  double savedr13;
  double savedr22;
  double savedr23;
  double orig_savedl12;
  double orig_savedl13;
  double orig_savedl22;
  double orig_savedl23;
  double orig_savedr12;
  double orig_savedr13;
  double orig_savedr22;
  double orig_savedr23;
  double tmp12;
  double tmp13;
  double tmp22;
  double tmp23;
  double dxx;
  double preaddp;
  // Ensure m11 + m21 <= m12 + m22 <= m13 + m23.
  uii = m11 + m21;
  ujj = m12 + m22;
  if (uii > ujj) {
    ukk = m11;
    m11 = m12;
    m12 = ukk;
    ukk = m21;
    m21 = m22;
    m22 = ukk;
    ukk = uii;
    uii = ujj;
    ujj = ukk;
  }
  ukk = m13 + m23;
  if (ujj > ukk) {
    ujj = ukk;
    ukk = m12;
    m12 = m13;
    m13 = ukk;
    ukk = m22;
    m22 = m23;
    m23 = ukk;
  }
  if (uii > ujj) {
    ukk = m11;
    m11 = m12;
    m12 = ukk;
    ukk = m21;
    m21 = m22;
    m22 = ukk;
  }
  // Ensure majority of probability mass is in front of m11.
  if ((((uint64_t)m11) * (m22 + m23)) > (((uint64_t)m21) * (m12 + m13))) {
    ukk = m11;
    m11 = m21;
    m21 = ukk;
    ukk = m12;
    m12 = m22;
    m22 = ukk;
    ukk = m13;
    m13 = m23;
    m23 = ukk;
  }
  if ((((uint64_t)m12) * m23) > (((uint64_t)m13) * m22)) {
    base_probr = cur_prob;
    savedr12 = m12;
    savedr13 = m13;
    savedr22 = m22;
    savedr23 = m23;
    tmp12 = savedr12;
    tmp13 = savedr13;
    tmp22 = savedr22;
    tmp23 = savedr23;
    // m12 and m23 must be nonzero
    dxx = tmp12 * tmp23;
    do {
      tmp13 += 1;
      tmp22 += 1;
      cur_prob *= dxx / (tmp13 * tmp22);
      tmp12 -= 1;
      tmp23 -= 1;
      if (cur_prob <= EXACT_TEST_BIAS) {
	tprob += cur_prob;
	break;
      }
      cprob += cur_prob;
      if (cprob == INFINITY) {
	return 0;
      }
      dxx = tmp12 * tmp23;
      // must enforce tmp12 >= 0 and tmp23 >= 0 since we're saving these
    } while (dxx > 0.5);
    savedl12 = tmp12;
    savedl13 = tmp13;
    savedl22 = tmp22;
    savedl23 = tmp23;
    base_probl = cur_prob;
    do {
      tmp13 += 1;
      tmp22 += 1;
      cur_prob *= (tmp12 * tmp23) / (tmp13 * tmp22);
      tmp12 -= 1;
      tmp23 -= 1;
      preaddp = tprob;
      tprob += cur_prob;
    } while (tprob > preaddp);
    tmp12 = savedr12;
    tmp13 = savedr13;
    tmp22 = savedr22;
    tmp23 = savedr23;
    cur_prob = base_probr;
    do {
      tmp12 += 1;
      tmp23 += 1;
      cur_prob *= (tmp13 * tmp22) / (tmp12 * tmp23);
      tmp13 -= 1;
      tmp22 -= 1;
      preaddp = tprob;
      tprob += cur_prob;
    } while (tprob > preaddp);
  } else {
    base_probl = cur_prob;
    savedl12 = m12;
    savedl13 = m13;
    savedl22 = m22;
    savedl23 = m23;
    if (!((((uint64_t)m12) * m23) + (((uint64_t)m13) * m22))) {
      base_probr = cur_prob;
      savedr12 = savedl12;
      savedr13 = savedl13;
      savedr22 = savedl22;
      savedr23 = savedl23;
    } else {
      tmp12 = savedl12;
      tmp13 = savedl13;
      tmp22 = savedl22;
      tmp23 = savedl23;
      dxx = tmp13 * tmp22;
      do {
	tmp12 += 1;
	tmp23 += 1;
	cur_prob *= dxx / (tmp12 * tmp23);
	tmp13 -= 1;
	tmp22 -= 1;
	if (cur_prob <= EXACT_TEST_BIAS) {
	  tprob += cur_prob;
	  break;
	}
	cprob += cur_prob;
	if (cprob == INFINITY) {
	  return 0;
	}
	dxx = tmp13 * tmp22;
      } while (dxx > 0.5);
      savedr12 = tmp12;
      savedr13 = tmp13;
      savedr22 = tmp22;
      savedr23 = tmp23;
      base_probr = cur_prob;
      do {
	tmp12 += 1;
	tmp23 += 1;
	cur_prob *= (tmp13 * tmp22) / (tmp12 * tmp23);
	tmp13 -= 1;
	tmp22 -= 1;
	preaddp = tprob;
	tprob += cur_prob;
      } while (tprob > preaddp);
      tmp12 = savedl12;
      tmp13 = savedl13;
      tmp22 = savedl22;
      tmp23 = savedl23;
      cur_prob = base_probl;
      do {
	tmp13 += 1;
	tmp22 += 1;
	cur_prob *= (tmp12 * tmp23) / (tmp13 * tmp22);
	tmp12 -= 1;
	tmp23 -= 1;
	preaddp = tprob;
	tprob += cur_prob;
      } while (tprob > preaddp);
    }
  }
  row_prob = tprob + cprob;
  orig_base_probl = base_probl;
  orig_base_probr = base_probr;
  orig_row_prob = row_prob;
  orig_savedl12 = savedl12;
  orig_savedl13 = savedl13;
  orig_savedl22 = savedl22;
  orig_savedl23 = savedl23;
  orig_savedr12 = savedr12;
  orig_savedr13 = savedr13;
  orig_savedr22 = savedr22;
  orig_savedr23 = savedr23;
  for (; dir < 2; dir++) {
    cur11 = m11;
    cur21 = m21;
    if (dir) {
      base_probl = orig_base_probl;
      base_probr = orig_base_probr;
      row_prob = orig_row_prob;
      savedl12 = orig_savedl12;
      savedl13 = orig_savedl13;
      savedl22 = orig_savedl22;
      savedl23 = orig_savedl23;
      savedr12 = orig_savedr12;
      savedr13 = orig_savedr13;
      savedr22 = orig_savedr22;
      savedr23 = orig_savedr23;
      ukk = m11;
      if (ukk > m22 + m23) {
	ukk = m22 + m23;
      }
    } else {
      ukk = m21;
      if (ukk > m12 + m13) {
	ukk = m12 + m13;
      }
    }
    ukk++;
    while (--ukk) {
      if (dir) {
	cur21 += 1;
	if (savedl23) {
	  savedl13 += 1;
	  row_prob *= (cur11 * (savedl22 + savedl23)) / (cur21 * (savedl12 + savedl13));
	  base_probl *= (cur11 * savedl23) / (cur21 * savedl13);
	  savedl23 -= 1;
	} else {
	  savedl12 += 1;
	  row_prob *= (cur11 * (savedl22 + savedl23)) / (cur21 * (savedl12 + savedl13));
	  base_probl *= (cur11 * savedl22) / (cur21 * savedl12);
	  savedl22 -= 1;
	}
	cur11 -= 1;
      } else {
	cur11 += 1;
	if (savedl12) {
	  savedl22 += 1;
	  row_prob *= (cur21 * (savedl12 + savedl13)) / (cur11 * (savedl22 + savedl23));
	  base_probl *= (cur21 * savedl12) / (cur11 * savedl22);
	  savedl12 -= 1;
	} else {
	  savedl23 += 1;
	  row_prob *= (cur21 * (savedl12 + savedl13)) / (cur11 * (savedl22 + savedl23));
	  base_probl *= (cur21 * savedl13) / (cur11 * savedl23);
	  savedl13 -= 1;
	}
	cur21 -= 1;
      }
      if (fisher23_tailsum(&base_probl, &savedl12, &savedl13, &savedl22, &savedl23, &dxx, 0)) {
	break;
      }
      tprob += dxx;
      if (dir) {
	if (savedr22) {
	  savedr12 += 1;
	  base_probr *= ((cur11 + 1) * savedr22) / (cur21 * savedr12);
	  savedr22 -= 1;
	} else {
	  savedr13 += 1;
	  base_probr *= ((cur11 + 1) * savedr23) / (cur21 * savedr13);
	  savedr23 -= 1;
	}
      } else {
	if (savedr13) {
	  savedr23 += 1;
	  base_probr *= ((cur21 + 1) * savedr13) / (cur11 * savedr23);
	  savedr13 -= 1;
	} else {
	  savedr22 += 1;
	  base_probr *= ((cur21 + 1) * savedr12) / (cur11 * savedr22);
	  savedr12 -= 1;
	}
      }
      fisher23_tailsum(&base_probr, &savedr12, &savedr13, &savedr22, &savedr23, &dyy, 1);
      tprob += dyy;
      cprob += row_prob - dxx - dyy;
      if (cprob == INFINITY) {
	return 0;
      }
    }
    if (!ukk) {
      continue;
    }
    savedl12 += savedl13;
    savedl22 += savedl23;
    if (dir) {
      while (1) {
	preaddp = tprob;
	tprob += row_prob;
	if (tprob <= preaddp) {
	  break;
	}
	cur21 += 1;
	savedl12 += 1;
	row_prob *= (cur11 * savedl22) / (cur21 * savedl12);
	cur11 -= 1;
	savedl22 -= 1;
      }
    } else {
      while (1) {
	preaddp = tprob;
	tprob += row_prob;
	if (tprob <= preaddp) {
	  break;
	}
	cur11 += 1;
	savedl22 += 1;
	row_prob *= (cur21 * savedl12) / (cur11 * savedl22);
	cur21 -= 1;
	savedl12 -= 1;
      }
    }
  }
  return tprob / (tprob + cprob);
}

void chi22_precomp_thresh(uint32_t m11, uint32_t m12, uint32_t m21, uint32_t m22, uint32_t* m11_minp, uint32_t* m11_maxp, uint32_t* tiel, uint32_t* tieh) {
  if (((!m11) || (!m22)) && ((!m12) || (!m21))) {
    // sum-0 row or column, no freedom at all
    *m11_minp = 0;
    *m11_maxp = 0;
    *tiel = m11;
    *tieh = m11;
    return;
  }
  double cur11 = ((int32_t)m11);
  double cur12 = ((int32_t)m12);
  double cur21 = ((int32_t)m21);
  double cur22 = ((int32_t)m22);
  double row1_sum = cur11 + cur12;
  double row2_sum = cur21 + cur22;
  double col1_sum = cur11 + cur21;
  double col2_sum = cur12 + cur22;
  double tot_recip = 1.0 / (row1_sum + row2_sum);
  double exp11 = row1_sum * col1_sum * tot_recip;
  double exp12 = row1_sum * col2_sum * tot_recip;
  double exp21 = row2_sum * col1_sum * tot_recip;
  double exp22 = row2_sum * col2_sum * tot_recip;
  double recipx11 = 1.0 / exp11;
  double recipx12 = 1.0 / exp12;
  double recipx21 = 1.0 / exp21;
  double recipx22 = 1.0 / exp22;
  // double cur_stat = (cur11 - exp11) * (cur11 - exp11) * recipx11 + (cur12 - exp12) * (cur12 - exp12) * recipx12 + (cur21 - exp21) * (cur21 - exp21) * recipx21 + (cur22 - exp22) * (cur22 - exp22) * recipx22;
  //
  // chisq statistic is just a quadratic function of cur11.  Expressing it as
  // ax^2 + bx + c (where x = cur11), we can determine that the minimizing
  // value is -b/(2a), and it follows that (-b/a) - [initial m11] is the equal
  // p-value cur11 on the other side.
  //
  // cur12 = row1_sum - cur11
  // cur21 = col1_sum - cur11
  // cur22 = (m22 - m11) + cur11
  //
  // (row1_sum - cur11 - exp12) * (row1_sum - cur11 - exp12)
  //   = (cur11 + exp12 - row1_sum) * (cur11 + exp12 - row1_sum)
  //   = cur11^2 - cur11 * (2(row1_sum - exp12)) + [constant]
  //
  // cur_stat = cur11^2 * (recipx11 + recipx12 + recipx21 + recipx22)
  //          - cur11 * (2recipx11 * exp11 + 2recipx12 * (row1_sum - exp12)
  //                   + 2recipx21 * (col1_sum - exp21)
  //                   + 2recipx22 * (exp22 + m11 - m22))
  //          + [constant]
  double coeff_a = recipx11 + recipx12 + recipx21 + recipx22;
  double half_coeff_nb = recipx11 * exp11 + recipx12 * (row1_sum - exp12) + recipx21 * (col1_sum - exp21) + recipx22 * (exp22 + cur11 - cur22);
  double cval = half_coeff_nb / coeff_a;
  int32_t ii;
  if (cur11 > cval + EPSILON) {
    *m11_maxp = m11;
    *tieh = m11;
    cval = EPSILON + (2 * cval - cur11);
    ii = (int32_t)cval;
    *m11_minp = ii + 1;
    if ((cval - ii) > (2 * EPSILON)) {
      *tiel = m11;
    } else {
      *tiel = ii;
    }
  } else if (cur11 < cval - EPSILON) {
    *tiel = m11;
    *m11_minp = m11 + 1;
    cval = EPSILON + (2 * cval - cur11);
    ii = (int32_t)cval;
    *m11_maxp = ii;
    if ((cval - ii) > (2 * EPSILON)) {
      *tieh = m11;
    } else {
      *tieh = ii;
    }
  } else {
    *m11_minp = 0;
    *m11_maxp = 0;
    *tiel = m11;
    *tieh = m11;
  }
}
