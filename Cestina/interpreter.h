#pragma once
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

struct Value {
    enum Type { INT, FLOAT, BOOL, STRING, NONE } type = NONE;
    long long i = 0;
    double f = 0.0;
    bool b = false;
    std::string s;
    static Value make_string(const std::string& str) { Value v; v.type = STRING; v.s = str; return v; }

   // std::string s;

   // static Value make_string(const std::string& str) { Value v; v.type = STRING; v.s = str; return v; }

    Value parseIdent();
    Value() = default;
    static Value make_int(long long x) { Value v; v.type = INT; v.i = x; return v; }
    static Value make_float(double x) { Value v; v.type = FLOAT; v.f = x; return v; }
    static Value make_bool(bool x) { Value v; v.type = BOOL; v.b = x; return v; }
    std::string toString() const {
        switch (type) {
        case INT: return std::to_string(i);
        case FLOAT: { std::ostringstream ss; ss << f; return ss.str(); }
        case BOOL: return b ? "pravda" : "nepravda";
        case STRING: return s;
        default: return "none";
        }
    }
};

class Interpreter {
public:
    Interpreter();
    std::string run(const std::string& code);

private:
    std::string src;
    size_t pos = 0;
    std::string output;

    // symbol table
    std::unordered_map<std::string, Value> vars;

    // Lexer/parser helpers
    void skipSpaces();
    bool startsWith(const std::string& s);
    std::string parseIdent();
    double parseNumber();
    // Expressions
    Value parseExpression();
    Value parseTerm();
    Value parseFactor();

    // Statements
    void parseStatements();
    void parseStatement();

    // utilities
    bool consumeIf(const std::string& s);
    void expect(const std::string& s);
};