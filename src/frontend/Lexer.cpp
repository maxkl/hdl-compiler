
#include "Lexer.h"

#include <iostream>

#include <shared/errors.h>

namespace HDLCompiler {

std::string Lexer::TokenType_to_string(Lexer::TokenType type) {
    switch (type) {
        case TokenType::None: return "(none)";
        case TokenType::EndOfFile: return "end of file";
        case TokenType::Identifier: return "identifier";
        case TokenType::Number: return "number";
        case TokenType::Dot: return "'.'";
        case TokenType::Comma: return "','";
        case TokenType::Semicolon: return "';'";
        case TokenType::Colon: return "':'";
        case TokenType::Equals: return "'='";
        case TokenType::AND: return "'&'";
        case TokenType::OR: return "'|'";
        case TokenType::XOR: return "'^'";
        case TokenType::NOT: return "'~'";
        case TokenType::LeftBrace: return "'{'";
        case TokenType::RightBrace: return "'}'";
        case TokenType::LeftBracket: return "'['";
        case TokenType::RightBracket: return "']'";
        case TokenType::LeftParenthesis: return "'('";
        case TokenType::RightParenthesis: return "')'";
        case TokenType::InKeyword: return "'in'";
        case TokenType::OutKeyword: return "'out'";
        case TokenType::BlockKeyword: return "'block'";
        default: return "INVALID";
    }
}

std::ostream& operator<<(std::ostream &stream, Lexer::TokenType type) {
    stream << Lexer::TokenType_to_string(type);
    return stream;
}

Lexer::Location::Location(unsigned long line, unsigned long column, std::string filename)
    : line(line), column(column), filename(std::move(filename)) {
    //
}

std::string Lexer::Location::to_string() const {
    return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
}

std::ostream& operator<<(std::ostream &stream, const Lexer::Location &location) {
    stream << location.to_string();
    return stream;
}

Lexer::Token::Token(TokenType type, Location location)
    : type(type), location(std::move(location)), number({0, 0}) {
    //
}

std::string Lexer::Token::to_string() const {
    std::string str;
    str += Lexer::TokenType_to_string(type);
    switch (type) {
        case TokenType::Identifier:
            str += " \"" + identifier + "\"";
            break;
        case TokenType::Number:
            str += " " + std::to_string(number.value) + "#" + std::to_string(number.width);
            break;
        default:
            break;
    }
    str += " at " + location.to_string();
    return str;
}

std::ostream& operator<<(std::ostream &stream, const Lexer::Token &token) {
    stream << token.to_string();
    return stream;
}

Lexer::Lexer(std::ifstream &f, std::string filename)
    : f(f), location(1, 0, std::move(filename)) {
    //
}

int Lexer::getc() {
    lastLocation = location;

    int c = f.get();

    if (c == '\n') {
        location.line++;
        location.column = 0;
    } else {
        location.column++;
    }

    return c;
}

void Lexer::ungetc() {
    f.unget();
    location = lastLocation;
}

Lexer::Token Lexer::readNextToken() {
    int c;

    do {
        c = getc();

        // Skip comments starting with '//'
        if (c == '/') {
            if (getc() == '/') {
                // Comments end with a line break
                do {
                    c = getc();
                } while (c != '\n');
            } else {
                ungetc();
            }
        }
    } while (isspace(c));

    if (isdigit(c)) {
        std::uint64_t num = 0;

        do {
            num *= 10;
            num += c - '0';
            c = getc();
        } while (isdigit(c));

        std::uint64_t width = 0;

        if (c == '#') {
            c = getc();

            do {
                width *= 10;
                width += c - '0';
                c = getc();
            } while (isdigit(c));

            if (width == 0) {
                throw CompilerError(location.to_string() + ": Number literal width is 0");
            }
        }

        if (width > 64) {
            throw CompilerError(location.to_string() + ": Number literal width > 64");
        }

        if (width > 0 && num > (1u << width) - 1) {
            std::cerr << location << ": Number literal value doesn't fit in specified width (warning)\n";
        }

        ungetc();

        Token token(TokenType::Number, location);
        token.number.value = num;
        token.number.width = width;

        return token;
    } else if (isalpha(c) || c == '_') {
        std::string str;

        do {
            str += char(c);
            c = getc();
        } while (isalpha(c) || isdigit(c) || c == '_');

        ungetc();

        if (str == "in") {
            return Token(TokenType::InKeyword, location);
        } else if (str == "out") {
            return Token(TokenType::OutKeyword, location);
        } else if (str == "block") {
            return Token(TokenType::BlockKeyword, location);
        }
        
        Token token(TokenType::Identifier, location);
        token.identifier = str;

        return token;
    } else if (c == '.') {
        return Token(TokenType::Dot, location);
    } else if (c == ',') {
        return Token(TokenType::Comma, location);
    } else if (c == ';') {
        return Token(TokenType::Semicolon, location);
    } else if (c == ':') {
        return Token(TokenType::Colon, location);
    } else if (c == '=') {
        return Token(TokenType::Equals, location);
    } else if (c == '&') {
        return Token(TokenType::AND, location);
    } else if (c == '|') {
        return Token(TokenType::OR, location);
    } else if (c == '^') {
        return Token(TokenType::XOR, location);
    } else if (c == '~') {
        return Token(TokenType::NOT, location);
    } else if (c == '{') {
        return Token(TokenType::LeftBrace, location);
    } else if (c == '}') {
        return Token(TokenType::RightBrace, location);
    } else if (c == '[') {
        return Token(TokenType::LeftBracket, location);
    } else if (c == ']') {
        return Token(TokenType::RightBracket, location);
    } else if (c == '(') {
        return Token(TokenType::LeftParenthesis, location);
    } else if (c == ')') {
        return Token(TokenType::RightParenthesis, location);
    } else if (c == EOF) {
        return Token(TokenType::EndOfFile, location);
    }

    throw CompilerError(location.to_string() + ": Invalid character '" + (char) c + "'");
}

}
