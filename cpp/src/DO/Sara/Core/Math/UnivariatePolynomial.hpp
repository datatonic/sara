#pragma once

#include <iostream>
#include <string>
#include <vector>


namespace DO { namespace Sara {

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
      return _coeff.size() - 1;
    }

    UnivariatePolynomial operator+(const UnivariatePolynomial& other) const
    {
      auto res = UnivariatePolynomial{};
      res._coeff = std::vector<coeff_type>(
          std::max(this->degree(), other.degree()) + 1, 0);
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

    UnivariatePolynomial operator-(const UnivariatePolynomial& other) const
    {
      auto res = UnivariatePolynomial{};
      res._coeff = std::vector<coeff_type>(
          std::max(this->degree(), other.degree()) + 1, 0);

      for (auto i = 0u; i < this->_coeff.size(); ++i)
        res[i] += (*this)[i];

      for (auto j = 0u; j < other._coeff.size(); ++j)
        res[j] -= other[j];

      return res;
    }

    UnivariatePolynomial operator*(const UnivariatePolynomial& other) const
    {
      auto res = UnivariatePolynomial{};
      res._coeff = std::vector<coeff_type>(this->degree() + other.degree() + 1, 0);
      for (auto i = 0u; i < this->_coeff.size(); ++i)
        for (auto j = 0u; j < other._coeff.size(); ++j)
        {
          const auto& a = (*this)[i];
          const auto& b = other[j];
          res[i + j] += a * b;
        }
      return res;
    }

    //! @brief Euclidean division.
    auto operator/(const UnivariatePolynomial& other) const
        -> std::pair<UnivariatePolynomial, UnivariatePolynomial>
    {
      if (degree() < other.degree())
        return {*this, {}};

      auto a = *this;
      const auto& b = other;

      auto q = UnivariatePolynomial{};
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

    UnivariatePolynomial operator-() const
    {
      auto res = *this;
      for (auto& c : res._coeff)
        c = -c;
      return res;
    }

    //! @brief Horner method evaluation.
    template <typename T>
    coeff_type operator()(const T& z) const
    {
      auto b = _coeff[degree()];
      for (auto i = 1u; i < _coeff.size(); ++i)
        b = _coeff[degree() - i] + b * z;
      return b;
    }

    auto to_string() const -> std::string
    {
      auto str = std::string{};
      for (auto i = 0u; i < _coeff.size(); ++i)
      {
        str += std::to_string(_coeff[degree() - i]) + " X^" +
               std::to_string(degree() - i);
        if (i < degree())
          str += " + ";
      }
      return str;
    }

    std::vector<coeff_type> _coeff;
  };


  class Monomial
  {
  public:
    Monomial() = default;

    auto pow(int e) const -> UnivariatePolynomial<double>
    {
      auto P = UnivariatePolynomial<double>{};
      P._coeff = std::vector<double>(exponent * e + 1, 0);
      P._coeff[exponent * e] = 1;
      return P;
    }

    int exponent{1};
  };

  constexpr auto Z = Monomial{};


  template <typename T>
  UnivariatePolynomial<T> operator*(const T& a, const UnivariatePolynomial<T>& b)
  {
    auto res = b;
    for (auto i = 0u; i < res._coeff.size(); ++i)
      res[i] *= a;
    return res;
  }


  template <typename T>
  UnivariatePolynomial<T> operator+(const Monomial& a, const T& b)
  {
    auto res = UnivariatePolynomial<T>{};
    res._coeff = std::vector<T>(a.exponent + 1, 0);
    res._coeff[a.exponent] = 1.;
    res._coeff[0] = b;
    return res;
  }

  template <typename T>
  UnivariatePolynomial<T> operator-(const Monomial& a, const T& b)
  {
    auto res = UnivariatePolynomial<T>{};
    res._coeff = std::vector<T>(a.exponent + 1, 0);
    res._coeff[a.exponent] = 1.;
    res._coeff[0] = -b;
    return res;
  }

  template <typename T>
  UnivariatePolynomial<T> operator*(const T& a, const Monomial& b)
  {
    auto res = UnivariatePolynomial<T>{};
    res._coeff.resize(b.exponent + 1);
    res._coeff[b.exponent] = a;
    return res;
  }

} /* namespace Sara */
} /* nmespace DO */
