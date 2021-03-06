#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


namespace DO::Sara::Univariate {

  //! @ingroup Core
  //! @defgroup Math
  //! @{

  template <typename Coeff>
  class UnivariatePolynomial
  {
  public:
    using coeff_type = Coeff;

    inline UnivariatePolynomial() = default;

    inline explicit UnivariatePolynomial(int degree)
      : _coeff(degree + 1, 0)
    {
    }

    const coeff_type& operator[](int i) const
    {
      return _coeff[i];
    }

    coeff_type& operator[](int i)
    {
      return _coeff[i];
    }

    int degree() const
    {
      return int(_coeff.size()) - 1;
    }

    UnivariatePolynomial operator+(const UnivariatePolynomial& other) const
    {
      auto res = UnivariatePolynomial{std::max(this->degree(), other.degree())};
      for (auto i = 0u; i < this->_coeff.size(); ++i)
        res[i] += (*this)[i];
      for (auto j = 0u; j < other._coeff.size(); ++j)
        res[j] += other[j];
      return res;
    }

    UnivariatePolynomial operator+(const coeff_type& other) const
    {
      auto res = *this;
      res._coeff[0] += other;
      return res;
    }

    UnivariatePolynomial operator-(const coeff_type& other) const
    {
      return (*this) + (-other);
    }

    UnivariatePolynomial operator-(const UnivariatePolynomial& other) const
    {
      auto res = UnivariatePolynomial{std::max(this->degree(), other.degree())};

      for (auto i = 0u; i < this->_coeff.size(); ++i)
        res[i] += (*this)[i];

      for (auto j = 0u; j < other._coeff.size(); ++j)
        res[j] -= other[j];

      return res;
    }

    UnivariatePolynomial operator*(const UnivariatePolynomial& other) const
    {
      auto res = UnivariatePolynomial{degree() + other.degree()};
      for (auto i = 0u; i < this->_coeff.size(); ++i)
        for (auto j = 0u; j < other._coeff.size(); ++j)
        {
          const auto& a = (*this)[i];
          const auto& b = other[j];
          res[i + j] += a * b;
        }
      return res;
    }

    auto remove_leading_zeros()
    {
      auto d = degree();
      while (std::abs(_coeff[d]) < std::numeric_limits<double>::epsilon())
        --d;
      _coeff.resize(d + 1);
    }

    //! @brief Euclidean division.
    auto operator/(const UnivariatePolynomial& other) const
        -> std::pair<UnivariatePolynomial, UnivariatePolynomial>
    {
      if (degree() < other.degree())
        return {*this, {}};

      auto a = *this;
      const auto& b = other;

      auto q = UnivariatePolynomial{degree() - other.degree()};
      q._coeff = std::vector<coeff_type>(degree() - other.degree() + 1, 0);

      auto qi = q;

      // Euclidean division.
      while (a.degree() >= b.degree())
      {
        qi._coeff[qi.degree()] = a[a.degree()] / b[b.degree()];

        a = a - b * qi;
        a._coeff.resize(a.degree());

        q = q + qi;

        qi._coeff.resize(qi.degree());
      }

      return {q, a};
    }

    auto operator/(const coeff_type& other) const
      -> UnivariatePolynomial
    {
      auto res = *this;
      for (auto& c: res._coeff)
        c /= other;
      return res;
    }

    UnivariatePolynomial operator-() const
    {
      auto res = *this;
      for (auto& c : res._coeff)
        c = -c;
      return res;
    }

    //! @brief Horner method evaluation.
    template <typename T>
    auto operator()(const T& x0) const -> decltype(coeff_type{} + T{})
    {
      if (x0 == T(0))
        return _coeff[0];

      using result_type = decltype(coeff_type{} + T{});
      auto b = result_type(_coeff[degree()]);
      for (auto i = 1u; i < _coeff.size(); ++i)
        b = _coeff[degree() - i] + b * x0;
      return b;
    }

    auto to_string() const -> std::string
    {
      auto str = std::string{};
      std::ostringstream oss;
      for (auto i = 0u; i < _coeff.size(); ++i)
      {
        oss << _coeff[degree() - i] <<  " X^" << (degree() - i);
        if (int(i) < degree())
          oss << " + ";
      }
      return oss.str();
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const UnivariatePolynomial& p)
    {
      os << p.to_string();
      return os;
    }

    std::vector<coeff_type> _coeff;
  };


  class Monomial
  {
  public:
    Monomial() = default;

    template <typename T>
    auto pow(int e) const -> UnivariatePolynomial<T>
    {
      auto P = UnivariatePolynomial<double>{};
      P._coeff = std::vector<double>(exponent * e + 1, 0);
      P._coeff[exponent * e] = 1;
      return P;
    }

    template <typename T>
    inline auto to_polynomial() const -> UnivariatePolynomial<T>
    {
      auto P = UnivariatePolynomial<T>{};
      P._coeff = std::vector<T>(exponent + 1, T(0));
      P[exponent] = T(1);
      return P;
    }

    int exponent{1};
  };

  constexpr auto X = Monomial{};
  constexpr auto Z = Monomial{};


  template <typename T>
  auto operator+(const Monomial& a, const T& b)
  {
    auto res = UnivariatePolynomial<T>{};
    res._coeff = std::vector<T>(a.exponent + 1, 0);
    res._coeff[a.exponent] = 1.;
    res._coeff[0] = b;
    return res;
  }

  template <typename T>
  auto operator-(const Monomial& a, const T& b)
  {
    return a + (-b);
  }

  template <typename T>
  auto operator*(const T& a, const Monomial& b)
  {
    auto res = UnivariatePolynomial<T>{};
    res._coeff.resize(b.exponent + 1);
    res._coeff[b.exponent] = a;
    return res;
  }

  template <typename T>
  auto operator*(const Monomial& a, const T& b)
  {
    return b * a;
  }

  template <typename T>
  auto operator*(const T& a, const UnivariatePolynomial<T>& b)
  {
    auto res = b;
    for (auto i = 0u; i < res._coeff.size(); ++i)
      res[i] *= a;
    return res;
  }

  template <typename T>
  auto operator*(const UnivariatePolynomial<T>& a, const T& b)
  {
    return b * a;
  }

  template <typename T>
  auto operator*(const UnivariatePolynomial<T>& P, const Monomial& Q)
  {
    return P * Q.to_polynomial<T>();
  }

  template <typename T>
  auto operator*(const Monomial& P, const UnivariatePolynomial<T>& Q)
  {
    return Q * P;
  }

  template <typename T>
  auto operator/(const UnivariatePolynomial<T>& P, const Monomial& Q)
  {
    return P / Q.to_polynomial<T>();
  }

  //! @}

} /* namespace DO::Sara::Univariate */
