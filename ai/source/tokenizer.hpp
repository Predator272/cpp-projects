#pragma once

#include <string>
#include <list>

class string_tokenizer
{
public:
	using string_type = std::string;
	using array_type = std::list<string_type>;
private:
	array_type _tokens;
public:
	array_type process(const string_type& input)
	{
		const auto tokens = tokenize(input);
		for (const auto& token : tokens)
		{
			if (find(this->_tokens, hash(token)).empty())
			{
				this->_tokens.push_back(token);
			}
		}
	}

	string_type find_one(size_t hash_value)
	{
		return find(this->_tokens, hash_value);
	}

	string_type find_all(std::vector<size_t> hash_values)
	{
		string_type result;
		for (const auto hash_value : hash_values)
		{
			result += this->find_one(hash_value);
		}
		return result;
	}
public:
	static array_type tokenize(const string_type& input)
	{
		array_type tokens;
		string_type token;
		for (auto value : input)
		{
			if (_is_space(value) || _is_punct(value) || _is_digit(value))
			{
				_push_and_clear(tokens, token);
				_push(tokens, string_type(1, value));
			}
			else
			{
				token.push_back(value);
			}
		}
		_push_and_clear(tokens, token);
		return tokens;
	}

	static size_t hash(const string_type& value)
	{
		static const std::hash<string_type> hasher;
		return hasher(value);
	}

	static string_type find(const array_type& tokens, size_t hash_value)
	{
		for (const auto& token : tokens)
		{
			if (hash(token) == hash_value)
			{
				return token;
			}
		}
		return {};
	}
private:
	static void _push_and_clear(array_type& array, string_type& value)
	{
		if (!value.empty())
		{
			_push(array, value);
			value.clear();
		}
	}

	static void _push(array_type& array, const string_type& value)
	{
		array.push_back(value);
	}

	static constexpr bool _is_space(const char value) noexcept
	{
		return value == ' ' || (value >= '\t' && value <= '\r');
	}

	static constexpr bool _is_punct(const char value) noexcept
	{
		return (value >= '!' && value <= '/') || (value >= ':' && value <= '@') || (value >= '[' && value <= '`') || (value >= '{' && value <= '~');
	}

	static constexpr bool _is_digit(const char value) noexcept
	{
		return value >= '0' && value <= '9';
	}
};
