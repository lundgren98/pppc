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

class ShowLine
{
public:
	std::vector<std::string> lines;

	void at(size_t line, size_t column)
	{
		std::cout << lines.at(line - 1) << '\n';
		while (column--)
			std::cout << ' ';
		std::cout << "^\n";
	}
};

ShowLine showline{};

// Types

namespace IntrincicType
{
	enum IntrincicType {
		None,
		Unknown,
		Integer,
		String,
	};

	std::string HumanReadable(IntrincicType type)
	{
		switch (type) {
			case None:
				return "None";
			case Unknown:
				return "Unknown";
			case Integer:
				return "Integer";
			case String:
				return "String";
		}
		std::cerr << "ERROR: No human readable version of this "
			     "intristic exists!";
		exit(EXIT_FAILURE);
	}
} // namespace IntrincicType

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
		Divide,
		Modulo,
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

std::string HumanReadable(Token::Type type)
{
	switch (type) {
		case Token::Integer:
			return "Integer";
		case Token::String:
			return "String";
		case Token::Add:
			return "Addition";
		case Token::Subtract:
			return "Subtraction";
		case Token::Multiply:
			return "Multiplication";
		case Token::Divide:
			return "Division";
		case Token::Modulo:
			return "Modulo";
		case Token::Assign:
			return "Assignment";
		case Token::Lesser:
			return "Less than comparison";
		case Token::Greater:
			return "Greater than comparison";
		case Token::Not:
			return "Logical Negation";
		case Token::BitAnd:
			return "Bitwise Multiplication";
		case Token::BitRighShift:
			return "Rightwards Bitshifting";
		case Token::BitLeftShift:
			return "Leftwards Bitshifting";
		case Token::Variable:
			return "Variable";
		case Token::Print:
		case Token::PrintLine:
			return "Printing";
		case Token::Conditional:
			return "Conditional";
		case Token::While:
			return "Recursive Conditional";
		case Token::Loop:
			return "Recursion";
		case Token::Do:
			return "Beget";
		case Token::Done:
			return "Begotten";
		case Token::End:
			return "End Of Line";
	}
	std::cerr
		<< "ERROR: No human readable version of this intristic exists!";
	exit(EXIT_FAILURE);
}

Token::Type getPrimaryTokenType(char key)
{
	switch (key) {
		case '*':
			return Token::Type::Multiply;
		case '/':
			return Token::Type::Divide;
		case '%':
			return Token::Type::Modulo;
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

void printErrorMessage(const Token& token, const std::string& errorMessage)
{
	std::cerr << token.lineNumber << ":" << token.startPos
		  << ": ERROR: " << errorMessage;
	showline.at(token.lineNumber, token.startPos);
}

void printExpectedNumberOfArguments(
	const Token& token, unsigned long expected, unsigned long actual,
	const std::string& errorMessage = ""
)
{
	std::cerr << token.lineNumber << ":" << token.startPos
		  << ": ERROR: " << HumanReadable(token.type) << " Expected "
		  << expected << " arguments but got " << actual << '\n'
		  << errorMessage;
	showline.at(token.lineNumber, token.startPos);
}

void printExpectedType(
	const Token& token, const IntrincicType::IntrincicType expected,
	const IntrincicType::IntrincicType actual,
	const std::string& errorMessage = ""
)
{
	std::cerr << token.lineNumber << ":" << token.startPos
		  << ": ERROR: " << HumanReadable(token.type) << " Expected "
		  << HumanReadable(expected) << " but got "
		  << HumanReadable(actual) << '\n'
		  << errorMessage;
	showline.at(token.lineNumber, token.startPos);
}

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
			std::cout << '[' << HumanReadable(token.type) << "] ";
		tokens.push(token);
	}
	if (debug)
		std::cout << "[NEWLINE]\n";
	return tokens;
}

// BEGIN AST

struct ExprArgs {
	IntrincicType::IntrincicType ret, lhs, rhs;
};

struct Expression {
	Token token;

	enum Type {
		Nullary,
		Unary,
		Binary,
	} type;

	ExprArgs args;
	std::shared_ptr<Expression> LHS;
	std::shared_ptr<Expression> RHS;
};

ExprArgs getArgsTypes(const Token& token)
{
	switch (token.type) {
		case Token::Type::End:
		case Token::Type::Do:
		case Token::Type::Done:
			return { IntrincicType::IntrincicType::None,
				 IntrincicType::IntrincicType::None,
				 IntrincicType::IntrincicType::None };
		case Token::Type::String:
			return { IntrincicType::IntrincicType::String,
				 IntrincicType::IntrincicType::None,
				 IntrincicType::IntrincicType::None };
		case Token::Type::Variable:
			return { IntrincicType::IntrincicType::Unknown,
				 IntrincicType::IntrincicType::None,
				 IntrincicType::IntrincicType::None };
		case Token::Type::Integer:
			return { IntrincicType::IntrincicType::Integer,
				 IntrincicType::IntrincicType::None,
				 IntrincicType::IntrincicType::None };
		case Token::Type::Not:
		case Token::Type::Print:
		case Token::Type::PrintLine:
			return { IntrincicType::IntrincicType::Integer,
				 IntrincicType::IntrincicType::Integer,
				 IntrincicType::IntrincicType::None };
		case Token::Type::Add:
		case Token::Type::BitAnd:
		case Token::Type::BitRighShift:
		case Token::Type::BitLeftShift:
		case Token::Type::Subtract:
		case Token::Type::Multiply:
		case Token::Type::Divide:
		case Token::Type::Modulo:
		case Token::Type::Lesser:
		case Token::Type::Greater:
		case Token::Type::Conditional:
		case Token::Type::While:
		case Token::Type::Loop:
		case Token::Type::Assign:
			return { IntrincicType::IntrincicType::Integer,
				 IntrincicType::IntrincicType::Integer,
				 IntrincicType::IntrincicType::Integer };
	}
	std::cerr
		<< "ERROR: Forgot to assing expression type to a token type!\n";
	exit(EXIT_FAILURE);
}

Expression::Type getExpressionType(const Token& token)
{
	switch (token.type) {
		/* nullary */
		case Token::Type::Do:
		case Token::Type::Done:
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
		case Token::Type::Divide:
		case Token::Type::Modulo:
		case Token::Type::Lesser:
		case Token::Type::Greater:
		case Token::Type::Conditional:
		case Token::Type::While:
		case Token::Type::Loop:
		case Token::Type::Assign:
			return Expression::Type::Binary;
	}
	std::cerr
		<< "ERROR: Forgot to assing expression type to a token type!\n";
	exit(EXIT_FAILURE);
}

void printASTDebugMessage(const std::shared_ptr<Expression>& expr)
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
		ExprArgs args = getArgsTypes(token);
		if (type == Expression::Type::Nullary) {
			stack.push(std::make_shared<Expression>(Expression{
				token, type, args, nullptr, nullptr }));
			if (debug)
				printASTDebugMessage(stack.top());
			continue;
		}
		size_t argc_expected
			= (type == Expression::Type::Binary) ? 2 : 1;
		if (stack.size() < argc_expected) {
			printExpectedNumberOfArguments(
				token, argc_expected, stack.size()
			);
			exit(EXIT_FAILURE);
		}
		if (type == Expression::Type::Unary) {
			auto lhs = stack.top();
			stack.pop();
			stack.push(std::make_shared<Expression>(Expression{
				token, type, args, lhs, nullptr }));
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
				token, type, args, lhs, rhs }));
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
	std::vector<Token>& ops, const std::shared_ptr<Expression>& expr,
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
	ops.push_back(expr->token);
}

std::vector<Token> OperationStack(
	std::stack<std::shared_ptr<Expression>> expressionStack,
	bool debug = false
)
{
	std::vector<Token> ops{};
	while (expressionStack.size()) {
		std::shared_ptr<Expression> expr = expressionStack.top();
		expressionStack.pop();
		if (debug)
			std::cout << "PARSER: [" << expr->token.identifyer
				  << "]\n\t";
		getOrderOfOperations(ops, expr, debug);
		if (debug)
			std::cout << '\n';
	}
	return ops;
}

// END PARSER

// BEGIN Type Checker

bool illegalType(const std::shared_ptr<Expression>& expr)
{
	if (!expr)
		return false;
	if (expr->LHS && expr->LHS->args.ret != expr->args.lhs) {
		printExpectedType(
			expr->token, expr->args.lhs, expr->LHS->args.ret
		);
		return true;
	}
	if (expr->RHS && expr->RHS->args.ret != expr->args.rhs) {
		printExpectedType(
			expr->token, expr->args.rhs, expr->RHS->args.ret
		);
		return true;
	}
	return false;
}

bool illegalEval(const std::shared_ptr<Expression>& expr)
{
	if (!expr)
		return false;
	switch (expr->token.type) {
		case Token::Type::Divide:
		case Token::Type::Modulo: {
			if (std::stoi(expr->RHS->token.identifyer) == 0) {
				printErrorMessage(
					expr->token,
					" Does not allow zero-valued RHS!\n"
				);
				return true;
			}
		}
		default:
			break;
	}
	return false;
}

void typechecker(
	std::stack<std::shared_ptr<Expression>> expressionStack,
	bool debug = false
)
{
	if (debug)
		std::cout << "TYPECHECKER\n";
	bool illegalExpression = false;
	while (expressionStack.size()) {
		std::queue<std::shared_ptr<Expression>> checkMe{};
		std::shared_ptr<Expression> expr = expressionStack.top();
		expressionStack.pop();
		checkMe.push(expr);
		while (checkMe.size()) {
			std::shared_ptr<Expression> node = checkMe.front();
			checkMe.pop();
			illegalExpression |= illegalType(node);
			illegalExpression |= illegalEval(node);
			if (node->LHS)
				checkMe.push(node->LHS);
			if (node->RHS)
				checkMe.push(node->LHS);
		}
	}
	if (illegalExpression)
		exit(EXIT_FAILURE);
}

// END TYPECHECKER

// BEGIN INTERPRETER
void interpreter(const std::vector<Token>& ops, bool debug = false)
{
	std::stack<int> sim_stack{};
	size_t instruction = 0;
	while (instruction < ops.size()) {
		Token op = ops.at(instruction);
		++instruction;
		if (debug)
			std::cout << '[' << op.identifyer << "]\n";
		switch (op.type) {
			case Token::Type::Integer: {
				sim_stack.push(std::stoi(op.identifyer));
				continue;
			}
			case Token::Type::PrintLine: {
				std::cout << sim_stack.top() << '\n';
				continue;
			}
			case Token::Type::Add: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs + rhs);
				continue;
			}
			case Token::Type::Subtract: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs - rhs);
				continue;
			}
			case Token::Type::Multiply: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs + rhs);
				continue;
			}
			case Token::Type::Divide: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs / rhs);
				continue;
			}
			case Token::Type::Modulo: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs % rhs);
				continue;
			}
			case Token::Type::BitAnd: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs & rhs);
				continue;
			}
			case Token::Type::BitLeftShift: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs << rhs);
				continue;
			}
			case Token::Type::BitRighShift: {
				int lhs = sim_stack.top();
				sim_stack.pop();
				int rhs = sim_stack.top();
				sim_stack.pop();
				sim_stack.push(lhs >> rhs);
				continue;
			}
			default: {
				printErrorMessage(op, "Not Implemented Yet!\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

int main()
{
	std::ifstream file("main.ppp");
	std::vector<std::string> lines{};
	for (std::string line; std::getline(file, line); lines.push_back(line))
		;
	file.close();
	showline.lines = lines;
	bool debugLexer = true;
	bool debugAST = true;
	bool debugParser = true;
	bool debugTypechecker = true;
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
	typechecker(ast, debugTypechecker);
	std::vector<Token> ops = OperationStack(ast, debugParser);
	std::cout << "SIMULATION:\n\n" << std::endl;
	interpreter(ops);
}
