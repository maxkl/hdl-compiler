
#pragma once

#include <string>
#include <fstream>
#include <cstdint>

namespace HDLCompiler {

class Lexer {
public:
    enum class TokenType {
        None,

        EndOfFile,

        Identifier,

        Number,

        Dot,
        Comma,
        Semicolon,
        Colon,
        Equals,
        AND,
        OR,
        XOR,
        NOT,

        LeftBrace,
        RightBrace,
        LeftBracket,
        RightBracket,
        LeftParenthesis,
        RightParenthesis,

        InKeyword,
        OutKeyword,
        BlockKeyword
    };

    static std::string TokenType_to_string(TokenType type);

    struct Location {
        unsigned long line;
        unsigned long column;
        std::string filename;

        explicit Location(unsigned long line = 0, unsigned long column = 0, std::string filename = "");
        std::string to_string() const;
    };

    friend std::ostream& operator<<(std::ostream &stream, const Location &location);

    struct Token {
        TokenType type;
        Location location;

        std::string identifier;

        struct {
            std::uint64_t value;
            std::uint64_t width;
        } number;

        Token(TokenType type, Location location);
        std::string to_string() const;
    };

    friend std::ostream& operator<<(std::ostream &stream, const Token &token);

    Lexer(std::ifstream &f, std::string filename);

    Token readNextToken();

private:
    std::ifstream &f;
    // int c;
    Location location;
    // int last_c;
    Location lastLocation;

    int getc();
    void ungetc();
};

std::ostream& operator<<(std::ostream &stream, Lexer::TokenType type);
std::ostream& operator<<(std::ostream &stream, const Lexer::Token &token);
std::ostream& operator<<(std::ostream &stream, const Lexer::Location &location);

}
