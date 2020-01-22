# include "tokenizer.hpp"
#include "node.hpp"

enum class TokenID {
	function,
	const_,	var, static_,
	if_, for_, struct_,

	double_colon,
	paren_open, paren_close,
	curly_open, curly_close,
	square_open, square_close,
	colon, semicolon,
	comma,
	plus_eq, minus_eq, mul_eq, div_eq,

	plus, minus, mul, div,
	not_eq_, double_eq, less_eq, greater_eq,
	equal, less, greater,
	
	number,

	text,

	identifiler,
	white_space,

	unknown,
};

enum class NodeID {
	func_decla,
	func_impl,
	page,
	func_block,
	func_args,
	func_args_block,
	expr_block,
	expr,
	func_call,
	plus, minus, mul,div,
	equal,
	primary, subprimary,
	identifiler,
	number,
};

int main() {


	std::ifstream ifs("test.ph");

	std::string file{ (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() };
	string_extractor extractor{ file };

	Tokenizer tokenizer{ file };
	tokenizer.add({
		Token::reserved_word(TokenID::function, "func", Symbols::identifier),
		Token::reserved_word(TokenID::const_, "const", Symbols::identifier),
		Token::reserved_word(TokenID::var, "var", Symbols::identifier),
		Token::reserved_word(TokenID::static_, "static", Symbols::identifier),
		Token::reserved_word(TokenID::if_, "if", Symbols::identifier),
		Token::reserved_word(TokenID::for_, "for", Symbols::identifier),
		Token::reserved_word(TokenID::struct_, "struct", Symbols::identifier),

		Token::symbol(TokenID::double_colon, "::"),
		Token::symbol(TokenID::paren_open, "("),
		Token::symbol(TokenID::paren_close, ")"),
		Token::symbol(TokenID::curly_open, "{"),
		Token::symbol(TokenID::curly_close, "}"),
		Token::symbol(TokenID::square_open, "["),
		Token::symbol(TokenID::square_close, "]"),
		Token::symbol(TokenID::colon, ":"),
		Token::symbol(TokenID::semicolon, ";"),
		Token::symbol(TokenID::comma, ","),
		Token::symbol(TokenID::plus_eq , "+="),
		Token::symbol(TokenID::minus_eq, "-="),
		Token::symbol(TokenID::mul_eq  , "*="),
		Token::symbol(TokenID::div_eq  , "/="),
		Token::symbol(TokenID::plus , "+"),
		Token::symbol(TokenID::minus, "-"),
		Token::symbol(TokenID::mul  , "*"),
		Token::symbol(TokenID::div  , "/"),
		Token::symbol(TokenID::not_eq_, "!="),
		Token::symbol(TokenID::double_eq  , "=="),
		Token::symbol(TokenID::less_eq  , "<="),
		Token::symbol(TokenID::greater_eq  , ">="),
		Token::symbol(TokenID::equal  , "="),
		Token::symbol(TokenID::less  , "<"),
		Token::symbol(TokenID::greater  , ">"),

		Token::number(TokenID::number     , Symbols::numeric_10),

		Token::paren_text(TokenID::text, '\"'),

		Token::identifier(TokenID::identifiler, Symbols::identifier),
		Token::identifier(TokenID::white_space, Symbols::white_space),

		Token::unknown(TokenID::unknown)
	});

	tokenizer.tokenize(TokenID::white_space);
	

	Node page{ NodeID::page };
	page.begin = tokenizer.tokens.begin();
	page.end = tokenizer.tokens.end();

	Node func_decla{ NodeID::func_decla };
	Node func_impl{ NodeID::func_impl };
	Node func_args_block{ NodeID::func_args_block };
	Node func_args{ NodeID::func_args };
	Node func_block{ NodeID::func_block };

	Node expr_block{ NodeID::expr_block };
	Node expr{ NodeID::expr };

	Node plus{ NodeID::plus };
	Node minus{ NodeID::minus };
	Node mul{ NodeID::mul };
	Node div{ NodeID::div };
	Node equal{ NodeID::equal };
	Node primary{ NodeID::primary };
	Node subprimary{ NodeID::subprimary };
	Node func_call{ NodeID::func_call };
	Node identifiler{ NodeID::identifiler };
	Node number{ NodeID::number };

	page.has(Node::Rule::match(func_impl, TokenID::function, TokenID::identifiler, func_args_block, func_block))
		.has(Node::Rule::match(func_decla, TokenID::function, TokenID::identifiler, func_args_block, TokenID::semicolon));

	func_args_block.is(Node::Rule::paren(TokenID::paren_open, func_args, TokenID::paren_close));
	func_args.is(Node::Rule::slice(expr, TokenID::comma));
	func_block.is(Node::Rule::paren(TokenID::curly_open, expr_block, TokenID::curly_close));

	expr_block.has(Node::Rule::paren(TokenID::curly_open, expr_block, TokenID::curly_close))
		.has(Node::Rule::until(expr, TokenID::semicolon));

	expr.has(Node::Rule::binary_operator(equal, expr, TokenID::equal, expr))
		.is(Node::Rule::match(subprimary, subprimary));
	
	subprimary.has(Node::Rule::binary_operator(plus, subprimary, TokenID::plus, subprimary))
		.has(Node::Rule::binary_operator(minus, subprimary, TokenID::minus, subprimary))
		.is(Node::Rule::match(primary, primary));

	primary.is(Node::Rule::binary_operator(mul, primary, TokenID::mul, primary))
		.is(Node::Rule::binary_operator(div, primary, TokenID::div, primary))
		.has(Node::Rule::paren(TokenID::paren_open, expr, TokenID::paren_close))
		.is(Node::Rule::endpoint(number, TokenID::number))
		.is(Node::Rule::match(func_call, TokenID::identifiler, func_args_block))
		.is(Node::Rule::endpoint(identifiler, TokenID::identifiler));

	page.parse();

	std::cout << page.fmt() << std::endl;

	return 0;
}

