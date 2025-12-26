#include "main.hpp"

static constexpr ai::value_type step = 0.0f;
static size_t array_index = std::numeric_limits<size_t>::max();

std::ostream& operator<<(std::ostream& output, const ai::array_type& array)
{
	output << "[";
	if (array.size())
	{
		for (const auto& [index, element] : std::views::enumerate(array))
		{
			output << std::format("\033[{}m{:+.2f}\033[0m,", ((index == array_index) ? 33 : ((element > step) ? 32 : 31)), element);
		}
		output << "\b";
	}
	output << "]";
	return output;
}

float to_float(const std::string_view& string) noexcept
{
	float value = 0.0f;
	std::from_chars(string.data(), string.data() + string.size(), value);
	return value;
}

template<typename type>
constexpr type normalize(type value, type min, type max, type newmin, type newmax) noexcept
{
	return newmin + ((value - min) / (max - min)) * (newmax - newmin);
}


ai::array_type to_bit_array(char value)
{
	ai::array_type result(CHAR_BIT);
	std::for_each(std::begin(result), std::end(result), [&](float& f) {
		f = ((value & 1) == 1) ? +1.0f : -1.0f;
		value >>= 1;
	});
	return result;
}

char from_bit_array(const ai::array_type& array)
{
	char value = 0, shift = 0;
	std::for_each(std::begin(array), std::end(array), [&](float f) {
		if (f > 0.0f)
		{
			value |= (((char)1) << shift);
		}
		shift++;
	});
	return value;
}

std::vector<ai::array_type> input_string(const string_tokenizer::string_type& string)
{
	//const auto tokens = string_tokenizer::tokenize(string);
	std::vector<ai::array_type> result;
	for (const auto& token : string)
	{
		result.push_back(to_bit_array(token));
	}
	return result;
}

string_tokenizer::string_type output_string(const std::vector<ai::array_type>& values)
{
	string_tokenizer::string_type result;
	for (const auto& value : values)
	{
		result += from_bit_array(value);
	}
	return result;
}



int main()
{
	setlocale(LC_ALL, "");

	static std::vector<std::pair<std::string, std::string>> dataset{
		{ "0", "0" },
		{ "0", "1" },
	};



	ai::network network(8, 8, 8);

	//std::ifstream input_file("data.bin", std::ios::binary);
	//if (input_file)
	//{
	//	input_file >> network;
	//	input_file.close();
	//}

	static const auto zero = to_bit_array(0);
	const auto target_error = 0.01f;
	for (size_t epoch = 0; epoch != std::numeric_limits<size_t>::max(); epoch++)
	{
		//static std::mt19937 generator(std::random_device{}());
		//std::shuffle(dataset.begin(), dataset.end(), generator);
		const auto error = std::reduce(dataset.cbegin(), dataset.cend(), ai::value_type(0), [&](auto value, const auto& sample) {
			const auto cast_input = input_string(sample.first);
			const auto cast_target = input_string(sample.second);
			for (size_t i = 0; i != cast_target.size() + 1; i++)
			{
				value += network.train({ i < cast_input.size() ? cast_input[i] : zero, i < cast_target.size() ? cast_target[i] : zero });
			}
			return value / sample.second.size();
		}) / dataset.size();

		const auto success = error < target_error;
		if (epoch % 1000 == 0 || success)
		{
			std::println("\033[0;0Hepoch {:<20} error {:.6f}", epoch, error);
			std::cout.flush();
			for (const auto& [input, target] : dataset)
			{
				std::vector<ai::array_type> output;
				const auto cast_input = input_string(input);
				const auto cast_target = input_string(target);
				for (size_t i = 0; i != 32; i++)
				{
					const auto temp = network.predict(i < cast_input.size() ? cast_input[i] : zero);
					if ((temp == zero).min() == true)
					{
						break;
					}
					output.push_back(temp);
				}
				std::cout << "\33[2K\r\"" << output_string(cast_input) << "\" : \"" << output_string(cast_target) << "\" : \"" << output_string(output) << "\"" << std::endl;
				std::cout.flush();
			}
			if (success)
			{
				break;
			}
		}
	}

	//std::ofstream output_file("data.bin", std::ios::binary);
	//if (output_file)
	//{
	//	output_file << network;
	//	output_file.close();
	//}

	while (true)
	{
		std::string input;
		std::cout << "input: ";
		std::getline(std::cin, input);

		std::vector<ai::array_type> output;
		const auto cast_input = input_string(input);
		for (size_t i = 0; i != 32; i++)
		{
			const auto temp = network.predict(i < cast_input.size() ? cast_input[i] : zero);
			if ((temp == zero).min() == true)
			{
				break;
			}
			output.push_back(temp);
		}
		std::cout << "output: \"" << output_string(output) << "\"" << std::endl;

		//ai::array_type input(0.0f, network.input_size());
		//std::vector<std::string> buffers;

		//while (true)
		//{
		//	for (size_t i = 0; i != input.size(); i++)
		//	{
		//		input[i] = to_float(i < buffers.size() ? buffers[i] : "");
		//	}
		//	array_index = std::max<size_t>(buffers.size(), 1) - 1;
		//	std::cout << "\033[2K\r" << input << std::flush;

		//	const char ch = _getch();

		//	if (buffers.empty()) buffers.push_back("");
		//	auto& current_buffer = buffers.back();

		//	if (ch == '\r')
		//	{
		//		break;
		//	}
		//	else if (ch == '\b')
		//	{
		//		if (!current_buffer.empty())
		//		{
		//			if (current_buffer.back() == '.')
		//			{
		//				current_buffer.pop_back();
		//			}
		//			current_buffer.pop_back();
		//		}
		//		else
		//		{
		//			buffers.pop_back();
		//			if (!buffers.empty() && !buffers.back().empty()) buffers.back().pop_back();
		//		}
		//	}
		//	else if (ch == ' ' || ch == ',')
		//	{
		//		if (buffers.size() < input.size())
		//		{
		//			buffers.push_back("");
		//		}
		//		else
		//		{
		//			break;
		//		}
		//	}
		//	else if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == '+')
		//	{
		//		const auto pos = current_buffer.find('.');
		//		const auto begin = (pos == std::string::npos) ? current_buffer.cend() : current_buffer.cbegin() + pos;
		//		if (std::count_if(begin, current_buffer.cend(), [](char ch) { return std::isdigit(ch); }) < 2)
		//		{
		//			if (ch == '-')
		//			{
		//				if (current_buffer.empty())
		//				{
		//					current_buffer += ch;
		//				}
		//				else if (!current_buffer.starts_with('-'))
		//				{
		//					current_buffer = ch + current_buffer;
		//				}
		//			}
		//			else if (ch == '+')
		//			{
		//				if (!current_buffer.empty() && current_buffer[0] == '-')
		//				{
		//					current_buffer = current_buffer.substr(1);
		//				}
		//			}
		//			else if (ch == '.')
		//			{
		//				if (current_buffer.find('.') == std::string::npos)
		//				{
		//					current_buffer += ch;
		//				}
		//			}
		//			else
		//			{
		//				current_buffer += ch;
		//				current_buffer = std::format("{}", to_float(current_buffer));
		//			}
		//		}
		//	}
		//}
		//array_index = std::numeric_limits<size_t>::max();
		//std::cout << "\033[2K\r" << input << " : " << network.predict(input) << std::endl;
	}
	return 0;
}
