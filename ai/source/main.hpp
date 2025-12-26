#pragma once

#include <print>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <span>
#include <cmath>
#include <random>
#include <numeric>
#include <ranges>
#include <functional>
#include <execution>
#include <valarray>
#include <chrono>
#include <conio.h>
#include "tokenizer.hpp"
#include "ai.hpp"

namespace ai
{
	class base_optimizer
	{
	public:
		virtual void update(const array_type& inputs, array_type& weights, value_type& bias, value_type gradient) noexcept = 0;
	};

	class sgd_optimizer
	{
	private:
		const value_type _speed;
	public:
		explicit sgd_optimizer(value_type speed = 0.0001) noexcept : _speed(speed)
		{
		}

		void update(value_type& output, value_type gradient) noexcept
		{
			output += this->_speed * gradient;
		}
	};

	class adam_optimizer
	{
	private:
		const value_type _speed, _b1, _b2;
		value_type _b1_pow, _b2_pow, _m, _v;
	public:
		adam_optimizer(value_type speed = 0.001, value_type b1 = 0.9, value_type b2 = 0.999) noexcept : _speed(speed), _b1(b1), _b2(b2), _m(0), _v(0), _b1_pow(1), _b2_pow(1)
		{
		}

		void update(value_type& output, value_type gradient) noexcept
		{
			this->_m = this->_b1 * this->_m + (1 - this->_b1) * gradient;
			this->_v = this->_b2 * this->_v + (1 - this->_b2) * gradient * gradient;

			this->_b1_pow *= this->_b1;
			this->_b2_pow *= this->_b2;

			const auto m_hat = this->_m / (1 - this->_b1_pow);
			const auto v_hat = this->_v / (1 - this->_b2_pow);

			output += this->_speed * m_hat / (std::sqrt(v_hat) + std::numeric_limits<value_type>::epsilon());
		}
	};

	class base_neuron
	{
	public:
		virtual float predict(const array_type& inputs) = 0;
		virtual array_type update(const array_type& inputs, float gradient) = 0;
		virtual size_t size() const noexcept = 0;
	};

	class base_layer
	{
	public:
		virtual array_type predict(const array_type& inputs) = 0;
		virtual array_type update(const array_type& inputs, const array_type& gradients) = 0;
		virtual size_t input_size() const noexcept = 0;
		virtual size_t output_size() const noexcept = 0;
	};

	class neuron : public base_neuron, public tanh
	{
	protected:
		array_type _weights;
		value_type _bias, _sum, _output, _output_weight;
		std::vector<adam_optimizer> _weights_optimizer;
		adam_optimizer _bias_optimizer, _output_weight_optimizer;
	public:
		explicit neuron(size_t size = 1) : _weights(size), _bias(random_real_value()), _sum(0), _output(0), _output_weight(random_real_value())
		{
			random_real_array(this->_weights);
			_weights_optimizer.resize(size);
		}

		size_t size() const noexcept override
		{
			return this->_weights.size();
		}

		void reset() noexcept
		{
			this->_output = 0;
		}

		float predict(const array_type& inputs) override
		{
			this->_sum = dot(inputs, this->_weights) + this->_bias + (this->_output * this->_output_weight);
			return this->activation(this->_sum);
		}

		array_type update(const array_type& inputs, float gradiend) override
		{
			gradiend *= this->derivative(this->_sum);
			const auto result = gradiend * this->_weights;
			for (auto&& [weight, value, optimizer] : std::views::zip(this->_weights, inputs, this->_weights_optimizer))
			{
				optimizer.update(weight, gradiend * value);
			}
			this->_bias_optimizer.update(this->_bias, gradiend);
			this->_output_weight_optimizer.update(this->_output_weight, gradiend * this->_output);
			return result;
		}
	};


	struct layer : public base_layer
	{
	private:
		std::vector<neuron> _neurons;
	public:
		explicit layer(size_t input_size = 1, size_t output_size = 1) noexcept : _neurons(output_size, neuron(input_size))
		{
		}

		array_type predict(const array_type& inputs) override
		{
			array_type outputs(this->_neurons.size());
			std::transform(std::execution::par_unseq, this->_neurons.begin(), this->_neurons.end(), std::begin(outputs), [&](auto& neuron) {
				return neuron.predict(inputs);
			});
			return outputs;
		}

		array_type update(const array_type& inputs, const array_type& gradients) override
		{
			auto zipped = std::views::zip(this->_neurons, gradients);
			return std::transform_reduce(std::execution::par_unseq, zipped.begin(), zipped.end(), array_type(0.0f, inputs.size()), std::plus(), [&](auto tuple) {
				const auto [neuron, gradient] = tuple;
				return neuron.update(inputs, gradient);
			});
		}

		void reset() noexcept
		{
			for (auto& neuron : this->_neurons) neuron.reset();
		}

		size_t input_size() const noexcept
		{
			return this->_neurons[0].size();
		}

		size_t output_size() const noexcept
		{
			return this->_neurons.size();
		}

		//friend std::ostream& operator<<(std::ostream&, const layer&);
		//friend std::istream& operator>>(std::istream&, layer&);
	};

	struct network
	{
	private:
		std::vector<layer> _layers;
	public:
		explicit network(size_t input_size, size_t output_size, size_t hidden_matrix_size = 0)
		{
			if (input_size < 1 || output_size < 1)
			{
				throw std::invalid_argument("invalid size");
			}
			const auto size = 1 + hidden_matrix_size;
			auto current_input_size = input_size;
			auto current_output_size = hidden_matrix_size > 0 ? hidden_matrix_size : output_size;
			this->_layers.resize(size);
			for (size_t i = 0; i != size; i++)
			{
				this->_layers[i] = layer(current_input_size, current_output_size);
				current_input_size = current_output_size;
				current_output_size = (i == size - 2) ? output_size : hidden_matrix_size;
			}
		}

		std::valarray<float> predict(const std::valarray<float>& values)
		{
			std::valarray<float> current(values);
			for (auto& layer : this->_layers)
			{
				current = layer.predict(current);
			}
			return current;
		}

		float train(const std::pair<array_type, array_type>& dataset)
		{
			std::vector<array_type> values;
			values.reserve(this->_layers.size() + 1);
			values.push_back(dataset.first);
			for (auto& layer : this->_layers)
			{
				values.push_back(layer.predict(values.back()));
			}
			auto outputs = values.back();
			values.pop_back();
			auto gradients = dataset.second - outputs;
			const auto error = (gradients * gradients).sum() / gradients.size();
			for (auto&& [layer, values] : std::ranges::reverse_view(std::views::zip(this->_layers, values)))
			{
				gradients = layer.update(values, gradients);
			}
			return error;
		}

		void reset() noexcept
		{
			for (auto& layer : this->_layers) layer.reset();
		}

		size_t input_size() const noexcept
		{
			return this->_layers.front().input_size();
		}

		size_t output_size() const noexcept
		{
			return this->_layers.back().output_size();
		}

		//friend std::ostream& operator<<(std::ostream&, const network&);
		//friend std::istream& operator>>(std::istream&, network&);
	};

	template<typename type>
	void bin_write(std::ostream& output_stream, const type& value)
	{
		output_stream.write((const char*)std::addressof(value), sizeof(value));
	}

	template<typename type>
	type bin_read(std::istream& input_stream)
	{
		type value{};
		input_stream.read((char*)std::addressof(value), sizeof(value));
		return value;
	}

	//std::ostream& operator<<(std::ostream& output_stream, const neuron& object)
	//{
	//	bin_write(output_stream, object._weights.size());
	//	for (auto element : object._weights) bin_write(output_stream, element);
	//	bin_write(output_stream, object._output_weight);
	//	bin_write(output_stream, object._bias);
	//	return output_stream;
	//}

	//std::istream& operator>>(std::istream& input_stream, neuron& object)
	//{
	//	object._weights.resize(bin_read<size_t>(input_stream));
	//	for (auto& element : object._weights) element = bin_read<float>(input_stream);
	//	object._output_weight = bin_read<float>(input_stream);
	//	object._bias = bin_read<float>(input_stream);
	//	return input_stream;
	//}

	//std::ostream& operator<<(std::ostream& output_stream, const layer& object)
	//{
	//	bin_write(output_stream, object._neurons.size());
	//	for (const auto& element : object._neurons) output_stream << element;
	//	return output_stream;
	//}

	//std::istream& operator>>(std::istream& input_stream, layer& object)
	//{
	//	object._neurons.resize(bin_read<size_t>(input_stream));
	//	for (auto& element : object._neurons) input_stream >> element;
	//	return input_stream;
	//}

	//std::ostream& operator<<(std::ostream& output_stream, const network& object)
	//{
	//	bin_write(output_stream, object._layers.size());
	//	for (const auto& element : object._layers) output_stream << element;
	//	return output_stream;
	//}

	//std::istream& operator>>(std::istream& input_stream, network& object)
	//{
	//	object._layers.resize(bin_read<size_t>(input_stream));
	//	for (auto& element : object._layers) input_stream >> element;
	//	return input_stream;
	//}
}
