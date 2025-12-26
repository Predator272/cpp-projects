#pragma once

#include <valarray>
#include <ostream>

namespace ai
{
	using value_type = float;
	using array_type = std::valarray<value_type>;

	value_type random_real_value() noexcept;
	array_type& random_real_array(array_type& array) noexcept;
	value_type dot(const array_type& a, const array_type& b) noexcept;

	class base_activation
	{
	public:
		virtual value_type activation(value_type) const noexcept = 0;
		virtual value_type derivative(value_type) const noexcept = 0;
	};

	class sigmoid : public base_activation
	{
	public:
		value_type activation(value_type) const noexcept override;
		value_type derivative(value_type) const noexcept override;
	};

	class tanh : public base_activation
	{
	public:
		value_type activation(value_type) const noexcept override;
		value_type derivative(value_type) const noexcept override;
	};

	class softplus : public base_activation
	{
	public:
		value_type activation(value_type) const noexcept override;
		value_type derivative(value_type) const noexcept override;
	};

	class swish : public base_activation
	{
	public:
		value_type activation(value_type) const noexcept override;
		value_type derivative(value_type) const noexcept override;
	};
}
