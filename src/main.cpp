#include <cctype>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <queue>
#include <regex>
#include <sstream>
#include <stack>
#include <string>

// TOKEN BEGIN
struct Token {
	enum Type {
		/* value types */
		Integer,
		String,
		/* unnamed ops */
		Add,
		Subtract,
		Multiply,
		Assign,
		Lesser,
		Greater,
		Not,
		BitAnd,
		BitRighShift,
		BitLeftShift,
		/* vars */
		Variable,
		/* keywords */
		Print,
		PrintLine,
		Conditional,
		While,
		Loop,
		Do,
		Done,
		End,
	} type;

	std::string identifyer;
	size_t startPos;
	unsigned lineNumber;
};

Token::Type getPrimaryTokenType(char key)
{
	switch (key) {
		case '*':
			return Token::Type::Multiply;
		case '+':
			return Token::Type::Add;
		case '-':
			return Token::Type::Subtract;
		case '=':
			return Token::Type::Assign;
		case '.':
			return Token::Type::Print;
		case ':':
			return Token::Type::PrintLine;
		case '<':
			return Token::Type::Lesser;
		case '>':
			return Token::Type::Greater;
		case '!':
			return Token::Type::Not;
		case '@':
			return Token::Type::While;
		case '$':
			return Token::Type::Loop;
		case '?':
			return Token::Type::Conditional;
	}
	return Token::Type::End;
}

Token::Type getSecondaryTokenType(char key)
{
	switch (key) {
		case '*':
			return Token::Type::BitAnd;
		case '<':
			return Token::Type::BitLeftShift;
		case '>':
			return Token::Type::BitRighShift;
	}
	return Token::Type::End;
}

Token::Type keywordTokenType(Token::Type orig, const std::string& keyword)
{
	if (keyword == "print")
		return Token::Type::Print;
	if (keyword == "not")
		return Token::Type::Not;
	if (keyword == "while")
		return Token::Type::While;
	if (keyword == "loop")
		return Token::Type::Loop;
	if (keyword == "if")
		return Token::Type::Conditional;
	if (keyword == "do")
		return Token::Type::Do;
	if (keyword == "done")
		return Token::Type::Done;
	return orig;
}

// TOKEN END

std::queue<Token>
lexer(const std::string& input, unsigned lineNumber, bool debug = false)
{
	if (debug)
		std::cout << "LEXER:\n\t";
	std::queue<Token> tokens{};
	for (size_t i = 0; i < input.size(); ++i) {
		Token token{};
		token.lineNumber = lineNumber;
		token.startPos = i;
		token.type = Token::Type::End;
		bool singleCharacterToken = false;
		switch (input[i]) {
			case ' ':
				continue;
			case '#':
				i = input.size();
				continue;
			case '"':
				token.type = Token::Type::String;
				++i;
				while (i < input.size() && input[i] != '"') {
					token.identifyer += input[i];
					++i;
				}
				break;
			case '_':
				if (i > input.size() - 2)
					continue;
				++i;
				token.type = getSecondaryTokenType(input[i]);
				if (token.type == Token::Type::End)
					continue;
				token.identifyer += input[i - 1];
				token.identifyer += input[i];
				break;
			default:
				token.type = getPrimaryTokenType(input[i]);
				if (token.type != Token::Type::End)
					singleCharacterToken = true;
		}
		/* Numbers */
		while (i < input.size() && std::isdigit(input[i])) {
			token.type = Token::Type::Integer;
			token.identifyer += input[i];
			++i;
		}
		/* words */
		while (i < input.size() && std::isalpha(input[i])) {
			token.type = Token::Type::Variable;
			token.identifyer += input[i];
			++i;
		}
		/* Single Character Tokens */
		if (singleCharacterToken)
			token.identifyer = input[i];
		else
			token.type = keywordTokenType(
				token.type, token.identifyer
			);
		if (debug)
			std::cout << '[' << token.identifyer << "] ";
		tokens.push(token);
	}
	if (debug)
		std::cout << "[NEWLINE]\n";
	return tokens;
}

// BEGIN AST
struct Expression {
	Token token;

	enum Type {
		Nullary,
		Unary,
		Binary,
	} type;

	std::shared_ptr<Expression> LHS;
	std::shared_ptr<Expression> RHS;
};

Expression::Type getExpressionType(const Token& token)
{
	switch (token.type) {
		/* nullary */
		case Token::Type::End:
		case Token::Type::Integer:
		case Token::Type::String:
		case Token::Type::Variable:
			return Expression::Type::Nullary;
		/* unary   */
		case Token::Type::Not:
		case Token::Type::Print:
		case Token::Type::PrintLine:
			return Expression::Type::Unary;
		/* binary  */
		case Token::Type::Add:
		case Token::Type::BitAnd:
		case Token::Type::BitRighShift:
		case Token::Type::BitLeftShift:
		case Token::Type::Subtract:
		case Token::Type::Multiply:
		case Token::Type::Lesser:
		case Token::Type::Greater:
		case Token::Type::Conditional:
		case Token::Type::While:
		case Token::Type::Loop:
		case Token::Type::Assign:
		case Token::Type::Do:
		case Token::Type::Done:
			return Expression::Type::Binary;
	}
	std::cerr
		<< "ERROR: Forgot to assing expression type to a token type!\n";
	exit(EXIT_FAILURE);
}

void printASTDebugMessage(std::shared_ptr<Expression> expr)
{
	std::cout << "AST:\n";
	std::cout << expr->token.identifyer << " ";
	if (expr->LHS)
		std::cout << "( " << expr->LHS->token.identifyer;
	if (expr->RHS)
		std::cout << " , " << expr->RHS->token.identifyer;
	if (expr->LHS)
		std::cout << " )";
	std::cout << '\n';
}

const std::stack<std::shared_ptr<Expression>>
AST(std::stack<Token> tokens, bool debug = false)
{
	std::stack<std::shared_ptr<Expression>> stack{};
	while (tokens.size()) {
		Token token = tokens.top();
		tokens.pop();
		Expression::Type type = getExpressionType(token);
		if (type == Expression::Type::Nullary) {
			stack.push(std::make_shared<Expression>(Expression{
				token, type, nullptr, nullptr }));
			if (debug)
				printASTDebugMessage(stack.top());
			continue;
		}
		size_t argc_expected
			= (type == Expression::Type::Binary) ? 2 : 1;
		if (stack.size() < argc_expected) {
			std::cerr << token.lineNumber << ":" << token.startPos
				  << ": ERORR: " << token.identifyer
				  << " Expects " << argc_expected
				  << " arguments, but " << stack.size()
				  << " where given.\n";
			exit(EXIT_FAILURE);
		}
		if (type == Expression::Type::Unary) {
			auto lhs = stack.top();
			stack.pop();
			stack.push(std::make_shared<Expression>(Expression{
				token, type, lhs, nullptr }));
			if (debug)
				printASTDebugMessage(stack.top());
			continue;
		}
		if (type == Expression::Type::Binary) {
			auto lhs = stack.top();
			stack.pop();
			auto rhs = stack.top();
			stack.pop();
			stack.push(std::make_shared<Expression>(Expression{
				token, type, lhs, rhs }));
			if (debug)
				printASTDebugMessage(stack.top());
		}
	}
	return stack;
}

// END AST

// BEGIN PARSER
/**
 * Post-Ordering Depth First Traversal
 */
void getOrderOfOperations(
	std::queue<Token>& ops, const std::shared_ptr<Expression>& expr,
	bool debug = false
)
{
	if (!expr) {
		std::cerr << "postOrdBinTreeTrav encountered a null node! This "
			     "should not be happening!";
		exit(EXIT_FAILURE);
	}
	if (expr->LHS) {
		getOrderOfOperations(ops, expr->LHS, debug);
	}
	if (expr->RHS) {
		getOrderOfOperations(ops, expr->RHS, debug);
	}
	if (debug)
		std::cout << expr->token.identifyer << " ";
	ops.push(expr->token);
}

std::stack<Token> OperationStack(
	std::stack<std::shared_ptr<Expression>> expressionStack,
	bool debug = false
)
{
	std::stack<Token> ops{};
	while (expressionStack.size()) {
		std::shared_ptr<Expression> expr = expressionStack.top();
		expressionStack.pop();
		std::queue<Token> opsQueue{};
		if (debug)
			std::cout << "PARSER: [" << expr->token.identifyer
				  << "]\n\t";
		getOrderOfOperations(opsQueue, expr, debug);
		if (debug)
			std::cout << '\n';
		while (opsQueue.size()) {
			ops.push(opsQueue.front());
			opsQueue.pop();
		}
	}
	return ops;
}

// END PARSER

int main()
{
	std::ifstream file("main.ppp");
	std::vector<std::string> lines{};
	for (std::string line; std::getline(file, line); lines.push_back(line))
		;
	file.close();
	bool debugLexer = true;
	bool debugAST = true;
	bool debugParser = true;
	unsigned lineNumber = 0;
	std::stack<Token> tokenStack{};
	for (const std::string& line : lines) {
		++lineNumber;
		if (line.empty())
			continue;
		std::queue<Token> tokenQueue
			= lexer(line, lineNumber, debugLexer);
		while (tokenQueue.size()) {
			tokenStack.push(tokenQueue.front());
			tokenQueue.pop();
		}
	}
	std::stack<std::shared_ptr<Expression>> ast = AST(tokenStack, debugAST);
	std::stack<Token> ops = OperationStack(ast, debugParser);
}
