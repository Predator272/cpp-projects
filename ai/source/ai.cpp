#include "ai.hpp"
#include <random>
#include <execution>
#include <algorithm>

namespace ai
{
	value_type random_real_value() noexcept
	{
		static std::mt19937 generator(std::random_device{}());
		return std::uniform_real_distribution<value_type>(value_type(-1.0), value_type(+1.0))(generator);
	}

	array_type& random_real_array(array_type& array) noexcept
	{
		std::for_each(std::execution::par_unseq, std::begin(array), std::end(array), [&](auto& value) {
			value = random_real_value();
		});
		return array;
	}

	value_type dot(const array_type& a, const array_type& b) noexcept
	{
		return std::transform_reduce(std::execution::unseq, std::begin(a), std::end(a), std::begin(b), value_type());
	}


	value_type sigmoid::activation(value_type x) const noexcept
	{
		return 1 / (1 + std::exp(-x));
	}

	value_type sigmoid::derivative(value_type x) const noexcept
	{
		const auto y = this->activation(x);
		return y * (1 - y);
	}

	value_type tanh::activation(value_type x) const noexcept
	{
		return std::tanh(x);
	}

	value_type tanh::derivative(value_type x) const noexcept
	{
		x = this->activation(x);
		return 1 - x * x;
	}

	value_type softplus::activation(value_type x) const noexcept
	{
		return std::log1p(std::exp(x));
	}

	value_type softplus::derivative(value_type x) const noexcept
	{
		return 1 / (1 + std::exp(-x));
	}

	value_type swish::activation(value_type x) const noexcept
	{
		return x / (1 + std::exp(-x));
	}

	value_type swish::derivative(value_type x) const noexcept
	{
		const auto sig = 1 / (1 + std::exp(-x));
		return x + sig * (1 - x);
	}
}
