#include <DO/Sara/Core/Math/JenkinsTraub.hpp>
#include <DO/Sara/Core/Math/NewtonRaphson.hpp>

#include <array>
#include <iomanip>
#include <iostream>


#define SHOW_DEBUG_LOG
#define LOG_DEBUG std::cout << "[" << __FUNCTION__ << ":" << __LINE__ << "] "


using namespace std;


namespace DO { namespace Sara {

  auto compute_root_moduli_lower_bound(const UnivariatePolynomial<double>& P)
      -> double
  {
    auto Q = P;
    Q[0] = -std::abs(Q[0]);
    for (int i = 1; i <= Q.degree(); ++i)
      Q[i] = std::abs(Q[i]);

    /*
     * Memento:
     *
     * If P(x) = 0 and x != 0, then with the triangle inequality:
     *   |a_0| <= |a_n| |x|^n + ... + |a_1| |x|
     *      0 <=  |a_n| |x|^n + ... + |a_1| |x| - |a_0|
     *      0 <=  Q(|x|)
     *
     * Q is increasing for x > 0 because Q' is positive.
     *
     * It suffices to find one root beta of Q because if
     * 0 == Q(beta) <= Q(|x|)
     *
     * Then beta <= |x| because Q is increasing.
     *
     * And we know beta > 0, because |a_0| is positive.
     * Otherwise 0 would already be a root of P.
     *
     * So for efficiency, we need to deflate P first.
     *
     */

#ifdef SHOW_DEBUG_LOG
    LOG_DEBUG << "Compute moduli lower bound" << std::endl;
    LOG_DEBUG << "P[X] = " << P << std::endl;
    LOG_DEBUG << "Q[X] = " << Q << std::endl;
#endif

    auto x = -Q[0];
    auto newton_raphson = NewtonRaphson<double>{Q};
    x = newton_raphson(x, 100, 1e-2);

#ifdef SHOW_DEBUG_LOG
    LOG_DEBUG << "Moduli lower bound = " << x << endl;
#endif

    return x;
  }

  auto quadratic_roots(UnivariatePolynomial<double>& P)
      -> std::array<std::complex<double>, 2>
  {
    const auto& a = P[2];
    const auto& b = P[1];
    const auto& c = P[0];
    const auto delta = std::complex<double>(b * b - 4 * a * c);

    return {(-b + std::sqrt(delta)) / (2 * a),
            (-b - std::sqrt(delta)) / (2 * a)};
  }


  auto QuadraticFactor::initialize(const UnivariatePolynomial<double>& P,
                                   double phase,
                                   bool recalculate_root_moduli_lower_bound)
      -> void
  {
    constexpr auto i = std::complex<double>{0, 1};

    if (recalculate_root_moduli_lower_bound)
      beta = compute_root_moduli_lower_bound(P);

    auto& [s1, s2] = roots;
    s1 = beta * std::exp(i * phase);
    s2 = std::conj(s1);

    const auto u = -2 * std::real(s1);
    const auto v = std::real(s1 * s2);

    polynomial = Z.pow<double>(2) + u * Z + v;

#ifdef SHOW_DEBUG_LOG
    LOG_DEBUG << "sigma[X] = " << polynomial << endl;
    LOG_DEBUG << "s1 = " << this->s1() << endl;
    LOG_DEBUG << "s2 = " << this->s2() << endl;
    LOG_DEBUG << "u = " << this->u() << endl;
    LOG_DEBUG << "v = " << this->v() << endl;
#endif
  }


  auto AuxiliaryVariables::update_target_polynomial_aux_vars(
      const UnivariatePolynomial<double>& P, const QuadraticFactor& sigma)
      -> void
  {
    P_div_sigma = P / sigma.polynomial;
    const auto& [Q_P, R_P] = P_div_sigma;

    const auto& [s1, s2] = sigma.roots;
    P_s1 = R_P(s1);
    P_s2 = R_P(s2);

    b = R_P[1];
    a = R_P[0] - b * sigma.u();

#ifdef SHOW_DEBUG_LOG
    LOG_DEBUG << "R_P = " << R_P << endl;
    LOG_DEBUG << "P(s1) = "
              << "P(" << s1 << ") = " << P_s1 << endl;
    LOG_DEBUG << "P(s2) = "
              << "P(" << s2 << ") = " << P_s2 << endl;
    LOG_DEBUG << "a = " << a << endl;
    LOG_DEBUG << "b = " << b << endl;
#endif
  }

  auto AuxiliaryVariables::update_shift_polynomial_aux_vars(
      const UnivariatePolynomial<double>& K0, const QuadraticFactor& sigma)
      -> void
  {
    K0_div_sigma = K0 / sigma.polynomial;
    const auto& [Q_K0, R_K0] = K0_div_sigma;

    const auto& [s1, s2] = sigma.roots;
    K0_s1 = R_K0(s1);
    K0_s2 = R_K0(s2);

    d = R_K0[1];
    c = R_K0[0] - d * sigma.u();

#ifdef SHOW_DEBUG_LOG
    LOG_DEBUG << "R_K0 = " << R_K0 << endl;
    LOG_DEBUG << "K0(s1) = "
              << "K0(" << s1 << ") = " << K0_s1 << endl;
    LOG_DEBUG << "K0(s2) = "
              << "K0(" << s2 << ") = " << K0_s2 << endl;
    LOG_DEBUG << "c = " << c << endl;
    LOG_DEBUG << "d = " << d << endl;
#endif
  }


  auto initial_shift_polynomial(const UnivariatePolynomial<double>& P)
      -> UnivariatePolynomial<double>
  {
    return derivative(P) / P.degree();
  }

  auto next_zero_shift_polynomial(const UnivariatePolynomial<double>& K0,
                                  const UnivariatePolynomial<double>& P)
      -> UnivariatePolynomial<double>
  {
    // The two formula below are identical but the former might not very stable
    // numerically...
    //
    // auto K1 = ((K0 - (K0(0) / P(0)) * P) / Z).first;
    auto K1 = ((K0 - K0(0)) / Z).first - (K0(0) / P(0)) * (P / Z).first;

    return K1;
  }

  auto next_linear_shift_polynomial(const UnivariatePolynomial<double>& K0,
                                    const UnivariatePolynomial<double>& P,
                                    double s_i, double P_si, double K0_si)
      -> UnivariatePolynomial<double>
  {
    auto K1 = ((K0 - K0_si) / (Z - s_i)).first -
              (K0_si / P_si) * ((P - P_si) / (Z - s_i)).first;

    /*
     *    p(s) * k0(z) - p(s) * k0(s) - k0(s) * p(z) + k0(s) * p(s)
     * =  p(s) * k0(z)                - k0(s) * p(z)
     * =  |p(s)  k0(s)|
     *    |p(z)  k0(z)|
     *
     * */

    return K1;
  }

  auto next_quadratic_shift_polymomial(const QuadraticFactor& sigma,
                                       const AuxiliaryVariables& aux)
    -> UnivariatePolynomial<double>
  {
    const auto& a = aux.a;
    const auto& b = aux.b;
    const auto& c = aux.c;
    const auto& d = aux.d;

    const auto& u = sigma.u();
    const auto& v = sigma.v();

    const auto c0 = b * c - a * d;
    const auto c1 = (a * a + u * a * b + v * b * b) / c0;
    const auto c2 = (a * c + u * a * d + v * b * d) / c0;

    const auto& Q_P = aux.P_div_sigma.first;
    const auto& Q_K0 = aux.K0_div_sigma.first;

    auto K1 = c1 * Q_K0 + (Z - c2) * Q_P + b;

#ifdef SHOW_DEBUG_LOG
    LOG_DEBUG << "K1 = " << K1 << std::endl;
#endif

    return K1;
  }


  auto next_quadratic_factor(QuadraticFactor& sigma,
                             const UnivariatePolynomial<double>& P,
                             const UnivariatePolynomial<double>& K0,
                             const AuxiliaryVariables& aux)
    -> QuadraticFactor
  {
    const auto& a = aux.a;
    const auto& b = aux.b;
    const auto& c = aux.c;
    const auto& d = aux.d;

    const auto& u = sigma.u();
    const auto& v = sigma.v();

    const auto b1 = -K0[0] / P[0];
    const auto b2 = -(K0[1] + b1 * P[1]) / P[0];

    const auto a1 = b * c - a * d;
    const auto a2 = a * c + u * a * d + v * b * d;

    const auto c2 = b1 * a2;
    const auto c3 = b1 * b1 * (a * a + u * a * b + v * b * b);

    const auto c4 = v * b2 * a1 - c2 - c3;
    const auto c1 = c * c + u * c * d + v * d * d +
                    b1 * (a * c + u * b * c + v * b * d) - c4;

    const auto delta_u = -(u * (c2 + c3) + v * (b1 * a1 + b2 * a2)) / c1;
    const auto delta_v = v * c4 / c1;

#ifdef SHOW_DEBUG_LOG
    LOG_DEBUG << "b1 = " << b1 << endl;
    LOG_DEBUG << "b2 = " << b2 << endl;

    LOG_DEBUG << "a1 = " << a1 << endl;
    LOG_DEBUG << "a2 = " << a2 << endl;

    LOG_DEBUG << "c2 = " << c2 << endl;
    LOG_DEBUG << "c3 = " << c3 << endl;

    LOG_DEBUG << "c4 = " << c4 << endl;
    LOG_DEBUG << "c1 = " << c1 << endl;
    LOG_DEBUG << "v * b2 * a1 = " << v * b2 * a1 << endl;

    LOG_DEBUG << "delta_u = " << delta_u << endl;
    LOG_DEBUG << "delta_v = " << delta_v << endl;
#endif

    auto sigma_next = sigma;
    sigma_next.polynomial[0] += delta_v;
    sigma_next.polynomial[1] += delta_u;

    return sigma_next;
  }


  auto JenkinsTraub::stage1() -> void
  {
    LOG_DEBUG << "P[X] = " << P << endl;
    LOG_DEBUG << "[STAGE 1] " << endl;

    K0 = initial_shift_polynomial(P);
    LOG_DEBUG << "[ITER] " << 0 << "  K[0] = " << K0 << endl;

    for (int i = 1; i < M; ++i)
    {
      K0 = next_zero_shift_polynomial(K0, P);
      LOG_DEBUG << "[ITER] " << i << "  K[" << i << "] = " << K0 << endl;
    }
  }

  // Apply fixed shift polynomial to determine convergence.
  auto JenkinsTraub::stage2() -> void
  {
    LOG_DEBUG << "[STAGE 2] " << endl;

    // Stage 2 must be able to determine the convergence.
    while (cvg_type == NoConvergence)
    {
      // Choose roots randomly on the circle of radius beta.
      sigma0.initialize(P, 49 * M_PI / 180., true);

      aux.update_target_polynomial_aux_vars(P, sigma0);

      auto t = std::array<std::complex<double>, 3>{{0., 0., 0}};
      auto v = std::array<double, 3>{0, 0, 0};

      // Determine convergence type.
      int i = M;
      for (; i < L; ++i)
      {
        aux.update_shift_polynomial_aux_vars(K0, sigma0);
        K1 = next_quadratic_shift_polymomial(sigma0, aux);
        sigma1 = next_quadratic_factor(sigma0, P, K0, aux);

        LOG_DEBUG << "[ITER] " << i << endl;
        LOG_DEBUG << "  K[" << i << "][X] = " << K0 << endl;

        t[0] = t[1];
        t[1] = t[2];
        t[2] = sigma0.s1() - aux.P_s1 / aux.K0_s1;

        v[0] = v[1];
        v[1] = v[2];
        v[2] = sigma1.v();

        K0 = K1;

        LOG_DEBUG << "  sigma0[X] = " << sigma0.polynomial << endl;
        LOG_DEBUG << "  sigma1[X] = " << sigma1.polynomial << endl;
        LOG_DEBUG << "  K[" << i << "] = " << K0 << endl;
        for (int k = 0; k < 3; ++k)
          LOG_DEBUG << "  t[" << k << "] = " << t[k] << endl;
        for (int k = 0; k < 3; ++k)
          LOG_DEBUG << "  v[" << k << "] = " << v[k] << endl;

        if (i < M + 3)
          continue;

        if (weak_convergence_predicate(t))
        {
          cvg_type = LinearFactor_;
          linear_factor.polynomial = Z - std::real(t[2]);

          LOG_DEBUG << "Convergence to linear factor" << endl;
          LOG_DEBUG << "  " << linear_factor.polynomial << endl;

          break;
        }

        if (weak_convergence_predicate(v))
        {
          cvg_type = QuadraticFactor_;
          sigma0 = sigma1;
          sigma0.roots = quadratic_roots(sigma0.polynomial);

          LOG_DEBUG << "Convergence to quadratic factor" << endl;

          break;
        }
      }

      L = i;

      // The while loop will keep going if cvg_type is NoConvergence.
    }
  }

  // Apply variable shift polynomial.
  auto JenkinsTraub::stage3_linear_factor() -> ConvergenceType
  {
    LOG_DEBUG << "[STAGE 3: Linear factor refinement] " << endl;

    LOG_DEBUG << "  linear_factor = " << linear_factor.polynomial << endl;

    int i = L;

    auto Q_P = UnivariatePolynomial<double>{};
    auto R_P = UnivariatePolynomial<double>{};

    auto si = -linear_factor.polynomial[0];
    auto P_si = double{};
    auto K0_si = double{};
    auto K1_si = double{};

    auto z = std::array<double, 3>{0., 0., 0.};

    // Determine convergence type.
    while (i < L + max_iter)
    {
      ++i;

      std::tie(Q_P, R_P) = P / linear_factor.polynomial;
      P_si = R_P(si);

      // calculate_next_shift_polynomial();
      K1 = next_linear_shift_polynomial(K0, P, si, P_si, K0_si);
      K1_si = K1(si);

      LOG_DEBUG << "[ITER] " << i << endl;
      LOG_DEBUG << "  K[" << i << "] = " << K0 << endl;

      si -= P_si / K1_si;
      linear_factor.polynomial[0] = -si;
      z[0] = z[1];
      z[1] = z[2];
      z[2] = si;

      LOG_DEBUG << "  R_P(s_i) = "
                << "R_P(" << si << ") = " << P_si << std::endl;
      LOG_DEBUG << "  P(s_i) = "
                << "P(" << si << ") = " << P(si) << std::endl;
      LOG_DEBUG << "  s[" << i << "] = " << si << endl;

      // Update K0.
      K0 = K1;

      if (std::isnan(z[2]))
      {
        LOG_DEBUG << "Stopping prematuraly at iteration " << i << endl;
        break;
      }

      if (i < L + 3)
        continue;

      if (nikolajsen_root_convergence_predicate(z))
      {
        LOG_DEBUG << "Converged at iteration " << i << endl;
        break;
      }
    }
    LOG_DEBUG << "L[X] = " << linear_factor.polynomial << endl;

    return ConvergenceType::LinearFactor_;
  }

  auto JenkinsTraub::stage3_quadratic_factor() -> ConvergenceType
  {
    LOG_DEBUG << "[STAGE 3: Quadratic factor refinement] " << endl;

    int i = L;

    auto z = std::array<double, 3>{0., 0., 0.};

    // Determine convergence type.
    while (i < L + max_iter)
    {
      ++i;

      sigma0 = sigma1;
      sigma0.roots = quadratic_roots(sigma0.polynomial);

      aux.update_target_polynomial_aux_vars(P, sigma0);
      aux.update_shift_polynomial_aux_vars(K0, sigma0);

      K1 = next_quadratic_shift_polymomial(sigma0, aux);
      sigma1 = next_quadratic_factor(sigma0, P, K0, aux);

      LOG_DEBUG << "[ITER] " << i << endl;
      LOG_DEBUG << "  K[" << i << "] = " << K0 << endl;
      LOG_DEBUG << "  Sigma[" << i << "] = " << sigma0.polynomial << endl;

      z[0] = z[1];
      z[1] = z[2];
      z[2] = std::abs(sigma0.roots[0]) + std::abs(sigma0.roots[1]);

      // Update K0.
      K0 = K1;

      if (std::isnan(z[2]))
      {
        LOG_DEBUG << "Stopping prematuraly at iteration " << i << endl;
        break;
      }

      if (i < L + 3)
        continue;

      if (nikolajsen_root_convergence_predicate(z))
      {
        LOG_DEBUG << "Converged at iteration " << i << endl;
        break;
      }
    }

    LOG_DEBUG << "Sigma[X] = " << sigma1.polynomial << endl;
    LOG_DEBUG << "s1 = " << sigma1.s1() << " P(s1) = " << P(sigma1.s1())
              << endl;
    LOG_DEBUG << "s2 = " << sigma1.s2() << " P(s2) = " << P(sigma1.s2())
              << endl;

    return ConvergenceType::QuadraticFactor_;
  }

} /* namespace Sara */
} /* namespace DO */
