#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <sstream>
#include <functional>
#include <optional>

struct string_extractor {
	std::string base;
	const char* iter;

	string_extractor(const std::string& str) :
		base(str),
		iter(base.c_str()) { }

	bool is_nop() const {
		return *iter == '\0';
	}
};

enum class TokenID;

struct Token {
	TokenID id;
	std::string_view text;

	template<size_t N>
	static auto number(const TokenID id, std::array<char, N> numeric) {
		return [id, numeric](string_extractor& ex) -> std::optional<Token> {
			const char* const begin = ex.iter;
			size_t size = 0;
			for (; *ex.iter != '\0'; ++size, ++ex.iter) {
				bool skip = [numeric, &ex]() {
					for (auto i : numeric) { if (i == *ex.iter) return true; }
					return false;
				}();
				if (!skip) { break; }
			}
			if (size > 0) return std::make_optional(Token{ id, std::string_view{ begin , size } });
			else return std::nullopt;
		};
	}

	template<size_t N>
	static auto identifier(const TokenID id, std::array<char, N> words) {
		return [id, words](string_extractor& ex) -> std::optional<Token> {
			const char* const begin = ex.iter;
			size_t size = 0;
			for (; *ex.iter != '\0'; ++size, ++ex.iter) {
				bool skip = [words, &ex]() {
					for (auto i : words) { if (i == *ex.iter) return true; }
					return false;
				}();
				if (!skip) { break; }
			}
			if (size > 0) return std::make_optional(Token{ id, std::string_view{ begin , size } });
			else return std::nullopt;
		};
	}

	static auto symbol(const TokenID id, std::string_view word) {
		return [id, word](string_extractor& ex) -> std::optional<Token> {
			const char* const begin = ex.iter;
			const char* iter = ex.iter;
			for (char c : word) {
				if (*iter != c || *iter == '\0') { return std::nullopt; }
				iter++;
			}
			ex.iter += word.length();
			return std::make_optional(Token{ id, std::string_view{ begin , word.length() } });
		};
	}

	static auto paren_text(const TokenID id, const char quotation) {
		return [id, quotation](string_extractor& ex) -> std::optional<Token> {
			const char* const begin = ex.iter;
			if (*begin != quotation) return std::nullopt;
			size_t size = 1;
			++ex.iter;
			for (; *ex.iter != '\0'; ++size, ++ex.iter) {
				bool skip = [quotation, &ex]() {
					if (*ex.iter == quotation && *(ex.iter - 1) != '\\') return false;
					return true;
				}();
				if (!skip) { ++ex.iter; ++size; break; }
			}
			return std::make_optional(Token{ id, std::string_view{ begin , size } });
		};
	}


	template<size_t N>
	static auto reserved_word(const TokenID id, std::string_view word, std::array<char, N> black_list) {
		return [id, word, black_list](string_extractor& ex) -> std::optional<Token> {
			const char* const begin = ex.iter;
			const char* iter = ex.iter;
			for (char c : word) {
				if (*iter != c || *iter == '\0') { return std::nullopt; }
				iter++;
			}
			for (auto c : black_list) if (*iter == c) { return std::nullopt; }
			ex.iter += word.length();
			return std::make_optional(Token{ id, std::string_view{ begin , word.length() } });
		};
	}

	static auto unknown(const TokenID id) {
		return [id](string_extractor& ex) -> std::optional<Token> {
			Token token{ id, std::string_view{ex.iter, 1} };
			ex.iter++;
			return token;
		};
	}
};

namespace Symbols {
	const std::array<char, 10> numeric_2{
			'0','1'
	};
	const std::array<char, 10> numeric_10{
			'0','1','2','3','4',
			'5','6','7','8','9'
	};
	const std::array<char, 16> numeric_16{
			'0','1','2','3','4',
			'5','6','7','8','9',
			'a','b','c','d','e','f'
	};
	const std::array<char, 63> identifier{
			'a','b','c','d','e','f','g','h','i',
			'j','k','l','m','n','o','p','q','r',
			's','t','u','v','w','x','y','z',
			'A','B','C','D','E','F','G','H','I',
			'J','K','L','M','N','O','P','Q','R',
			'S','T','U','V','W','X','Y','Z',
			'0','1','2','3','4','5','6','7','8','9',
			'_'
	};
	const std::array<char, 26> lower{
			'a','b','c','d','e','f','g','h','i',
			'j','k','l','m','n','o','p','q','r',
			's','t','u','v','w','x','y','z'
	};
	const std::array<char, 26> upper{
			'A','B','C','D','E','F','G','H','I',
			'J','K','L','M','N','O','P','Q','R',
			'S','T','U','V','W','X','Y','Z'
	};
	const std::array<char, 10> white_space{
			' ','\t','\n','\r'
	};
	const std::array<char, 0> nothing;
}


struct Tokenizer {
	string_extractor extractor;
	
	using tokenizer_func = std::function<std::optional<Token>(string_extractor&)>;
	std::vector<tokenizer_func> tokenizers;

	std::vector<Token> tokens;

	Tokenizer(const std::string& file) : extractor(file){

	}

	void tokenize(TokenID ignore) {
		tokens.clear();
		while (!extractor.is_nop()) {
			for (auto& tokenizer : tokenizers) {
				auto token = tokenizer(extractor);
				if (token) {
					if (token.value().id != ignore) {
						tokens.push_back(*token);
						//std::cout << token.value().text << "\033[32m:" << (int)token->id << "\033[m" << std::endl;
					}
					break;
				}
			}
		}
	}

	void add(tokenizer_func& f) {
		tokenizers.push_back(f);
	}

	void add(std::vector<tokenizer_func> f) {
		std::copy(f.begin(), f.end(), std::back_inserter(tokenizers));
	}
};