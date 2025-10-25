#include "interpreter.h"
#include <cctype>
#include <sstream>
#include <cmath>
/*
std::string Value::toString() const {
    switch (type) {
    case INT: return std::to_string(i);
    case FLOAT: {
        std::ostringstream ss; ss << f; return ss.str();
    }
    case BOOL: return b ? "true" : "false";
    default: return "none";
    }
}
*/
Interpreter::Interpreter() {}

std::string Interpreter::run(const std::string& code) {
    src = code;
    pos = 0;
    output.clear();
    vars.clear();
    try {
        parseStatements();
    }
    catch (const std::string& e) {
        output += std::string("Chyba: ") + e + "           ";
    }
    return output;
}

void Interpreter::skipSpaces() {
    while (pos < src.size() && std::isspace((unsigned char)src[pos])) pos++;
}

bool Interpreter::startsWith(const std::string& s) {
    skipSpaces();
    return src.substr(pos, s.size()) == s;
}

bool Interpreter::consumeIf(const std::string& s) {
    skipSpaces();
    if (src.substr(pos, s.size()) == s) { pos += s.size(); return true; }
    return false;
}

void Interpreter::expect(const std::string& s) {
    skipSpaces();
    if (src.substr(pos, s.size()) != s) throw std::string("Ocekavano '") + s + "'";
    pos += s.size();
}

std::string Interpreter::parseIdent() {
    skipSpaces();

    size_t start = pos;
    if (pos < src.size() && (std::isalpha((unsigned char)src[pos]) || src[pos] == '_')) {
        pos++;
        while (pos < src.size() && (std::isalnum((unsigned char)src[pos]) || src[pos] == '_')) pos++;
        return src.substr(start, pos - start);
    }
    throw std::string("Ocekavan identifikator");
}

double Interpreter::parseNumber() {
    skipSpaces();
    size_t start = pos;
    bool dot = false;
    if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) pos++;
    while (pos < src.size() && (std::isdigit((unsigned char)src[pos]) || src[pos] == '.')) {
        if (src[pos] == '.') dot = true;
        pos++;
    }
    std::string tok = src.substr(start, pos - start);
    if (tok.empty()) throw std::string("Ocekavano cislo");
    return std::stod(tok);
}

// Expr := Term { (+|-) Term }
Value Interpreter::parseExpression() {
    Value v = parseTerm();
    while (true) {
        skipSpaces();
        if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) {
            char op = src[pos++];
            Value r = parseTerm();
            // promote to float if needed
            if (v.type == Value::FLOAT || r.type == Value::FLOAT) {
                double lv = (v.type == Value::FLOAT) ? v.f : (v.type == Value::INT ? (double)v.i : 0.0);
                double rv = (r.type == Value::FLOAT) ? r.f : (r.type == Value::INT ? (double)r.i : 0.0);
                v = Value::make_float(op == '+' ? lv + rv : lv - rv);
            }
            else {
                long long lv = v.i; long long rv = r.i;
                v = Value::make_int(op == '+' ? lv + rv : lv - rv);
            }
        }
        else break;
    }
    return v;
}

// Term := Factor { (*|/) Factor }
Value Interpreter::parseTerm() {
    Value v = parseFactor();
    while (true) {
        skipSpaces();
        if (pos < src.size() && src[pos] == '(') { pos++; Value v = parseExpression(); expect(")"); return v; }

        if (pos < src.size() && (src[pos] == '*' || src[pos] == '/')) {
            char op = src[pos++];
            Value r = parseFactor();
            if (v.type == Value::FLOAT || r.type == Value::FLOAT || op == '/') {
                double lv = (v.type == Value::FLOAT) ? v.f : (double)v.i;
                double rv = (r.type == Value::FLOAT) ? r.f : (double)r.i;
                v = Value::make_float(op == '*' ? lv * rv : lv / rv);
            }
            if (pos < src.size() && (std::isdigit((unsigned char)src[pos]) || src[pos] == '+' || src[pos] == '-')) {
                double num = parseNumber();
                if (std::floor(num) == num) return Value::make_int((long long)num);
                return Value::make_float(num);
            }
            else {
                long long lv = v.i; long long rv = r.i;
                v = Value::make_int(op == '*' ? lv * rv : (rv == 0 ? 0 : lv / rv));
            }
            std::string id = parseIdent();
        }
        else break;
    }
    return v;
}

// Factor := number | identifier | (expr) | boolean literals

/*
Value Interpreter::parseFactor() {
    skipSpaces();
    if (pos < src.size() && src[pos] == '(') { pos++; Value v = parseExpression(); expect(")"); return v; }

    // number
    if (pos < src.size() && (std::isdigit((unsigned char)src[pos]) || src[pos] == '+' || src[pos] == '-')) {
        double num = parseNumber();
        if (std::floor(num) == num) return Value::make_int((long long)num);
        return Value::make_float(num);
    }

    // identifier or keyword
    std::string id = parseIdent();
    if (id == "pravda") return Value::make_bool(true);
    if (id == "nepravda") return Value::make_bool(false);
    // variable lookup
    if (vars.find(id) != vars.end()) return vars[id];
    throw std::string("Neznamy identifikator: ") + id;
}
*/
Value Interpreter::parseFactor() {
    skipSpaces();
    if (pos < src.size() && src[pos] == '(') { pos++; Value v = parseExpression(); expect(")"); return v; }

    // string literal
    if (pos < src.size() && (src[pos] == '"' || src[pos] == '\'')) {
        char quote = src[pos++];
        std::string s;
        while (pos < src.size() && src[pos] != quote) {
            if (src[pos] == '\\' && pos + 1 < src.size()) {
                char next = src[++pos];
                if (next == 'n') s += '\n';
                else if (next == 't') s += '\t';
                else s += next;
                pos++;
            }
            else {
                s += src[pos++];
            }
        }
        if (pos >= src.size() || src[pos] != quote)
            throw std::string("Neukonceny retezec");
        pos++;
        return Value::make_string(s);
    }

    // number
    if (pos < src.size() && (std::isdigit((unsigned char)src[pos]) || src[pos] == '+' || src[pos] == '-')) {
        double num = parseNumber();
        if (std::floor(num) == num) return Value::make_int((long long)num);
        return Value::make_float(num);
    }

    // identifier or keyword
    std::string id = parseIdent();
    if (id == "pravda") return Value::make_bool(true);
    if (id == "nepravda") return Value::make_bool(false);
    if (vars.find(id) != vars.end()) return vars[id];
    throw std::string("Neznamy identifikator: ") + id;
}


// Statements: jednoduché sekvence oddìlené èárkami/novými øádky/bez nich — budeme akceptovat støedník na konci
void Interpreter::parseStatements() {
    while (true) {
        skipSpaces();
        if (pos >= src.size()) break;
        parseStatement();
    }
}

void Interpreter::parseStatement() {
    skipSpaces();
    if (startsWith("cele_cislo") || startsWith("plout") || startsWith("boolean")) {
        // deklarace: typ ident [= expr] ;
        std::string typ = parseIdent();
        std::string name = parseIdent();
        skipSpaces();
        if (consumeIf("=")) {
            Value val = parseExpression();
            if (typ == "cele_cislo") {
                if (val.type == Value::FLOAT) vars[name] = Value::make_int((long long)val.f);
                else if (val.type == Value::INT) vars[name] = val;
                else throw std::string("Typova chyba pri prirazeni do cele_cislo");
            }
            else if (typ == "plout") {
                if (val.type == Value::FLOAT) vars[name] = val;
                else if (val.type == Value::INT) vars[name] = Value::make_float((double)val.i);
                else throw std::string("Typova chyba pri prirazeni do plout");
            }
            else if (typ == "boolean") {
                if (val.type == Value::BOOL) vars[name] = val;
                else throw std::string("Typova chyba pri prirazeni do boolean");
            }
        }
        else {
            // default init
            if (typ == "cele_cislo") vars[name] = Value::make_int(0);
            else if (typ == "plout") vars[name] = Value::make_float(0.0);
            else if (typ == "boolean") vars[name] = Value::make_bool(false);
        }
        // optional semicolon
        consumeIf(";");
        return;
    }

    // tiskni expr ;
    if (startsWith("tiskni")) {
        parseIdent(); // consume 'tiskni'
        Value v = parseExpression();
        output += v.toString();
        output += "            ";
            consumeIf(";");
        return;
    }

    // pøiøazení: ident = expr ;
    if (std::isalpha((unsigned char)src[pos]) || src[pos] == '_') {
        std::string id = parseIdent();
        skipSpaces();
        if (consumeIf("=")) {
            Value val = parseExpression();
            if (vars.find(id) == vars.end()) throw std::string("Promenna neexistuje: ") + id;
            // jednoduchné pøetypování podobnì jako pøi deklaraci
            Value& dest = vars[id];
            if (dest.type == Value::INT) {
                if (val.type == Value::FLOAT) dest = Value::make_int((long long)val.f);
                else if (val.type == Value::INT) dest = val;
                else throw std::string("Typova chyba pri prirazeni");
            }
            else if (dest.type == Value::FLOAT) {
                if (val.type == Value::FLOAT) dest = val;
                else if (val.type == Value::INT) dest = Value::make_float((double)val.i);
                else throw std::string("Typova chyba pri prirazeni");
            }
            else if (dest.type == Value::BOOL) {
                if (val.type == Value::BOOL) dest = val; else throw std::string("Typova chyba pri prirazeni boolean");
            }
            consumeIf(";");
            return;
        }
    }

    // if (pokud) a while (zatimco) bloky — jednoduchý tvar: "pokud ( expr ) { ... } [jinak { ... }]"
    if (startsWith("pokud")) {
        consumeIf("pokud"); expect("("); Value cond = parseExpression(); expect(")"); expect("{");
        // capture block
        size_t block_start = pos; int nest = 1;
        while (pos < src.size() && nest>0) {
            if (src[pos] == '{') { nest++; pos++; }
            else if (src[pos] == '}') { nest--; if (nest == 0) break; pos++; }
            else pos++;
        }
        size_t block_end = pos; expect("}");
        std::string then_block = src.substr(block_start, block_end - block_start);
        std::string else_block;
        skipSpaces();
        if (startsWith("jinak")) {
            consumeIf("jinak"); expect("{"); size_t es = pos; int nest2 = 1;
            while (pos < src.size() && nest2>0) {
                if (src[pos] == '{') { nest2++; pos++; }
                else if (src[pos] == '}') { nest2--; if (nest2 == 0) break; pos++; }
                else pos++;
            }
            size_t ee = pos; expect("}");
            else_block = src.substr(es, ee - es);
        }
        if ((cond.type == Value::INT && cond.i != 0) || (cond.type == Value::FLOAT && cond.f != 0.0) || (cond.type == Value::BOOL && cond.b)) {
            Interpreter sub;
            sub.vars = this->vars; // copy variables
            output += sub.run(then_block);
            this->vars = sub.vars; // get changes back
        }
        else if (!else_block.empty()) {
            Interpreter sub;
            sub.vars = this->vars;
            output += sub.run(else_block);
            this->vars = sub.vars;
        }
        return;
    }

    if (startsWith("zatimco")) {
        consumeIf("zatimco"); expect("("); // jednoduché zatimco (condition) {block}
        size_t cond_start = pos; // parse condition as expression until )
        // naive: find )
        int p = 0; while (pos < src.size()) { if (src[pos] == ')') break; pos++; }
        std::string cond_expr = src.substr(cond_start, pos - cond_start);
        expect(")"); expect("{"); size_t block_start = pos; int nest = 1;
        while (pos < src.size() && nest>0) {
            if (src[pos] == '{') { nest++; pos++; }
            else if (src[pos] == '}') { nest--; if (nest == 0) break; pos++; }
            else pos++;
        }
        size_t block_end = pos; expect("}");
        std::string block = src.substr(block_start, block_end - block_start);
        // loop: evaluate cond by running a small interpreter on cond_expr that returns expression value
        while (true) {
            Interpreter condInterp; condInterp.vars = this->vars;
            // build a tiny program: an expression as a return of "t" - but our interpreter can't return expressions directly
            // so we embed: cele_cislo __cond = ( ... );
            std::string prog = std::string("cele_cislo __tmp = ") + cond_expr + ";";
            condInterp.run(prog);
            if (condInterp.vars.find("__tmp") == condInterp.vars.end()) break;
            Value cv = condInterp.vars["__tmp"];
            bool truth = (cv.type == Value::INT && cv.i != 0) || (cv.type == Value::FLOAT && cv.f != 0.0);
            if (!truth) break;
            Interpreter sub; sub.vars = this->vars;
            output += sub.run(block);
            this->vars = sub.vars;
        }
        return;
    }

    // otherwise try to skip a token to avoid infinite loop
    // try semicolon
    if (consumeIf(";")) return;
    // unknown token
    throw std::string("Neznamy statement pri pozici: ") + std::to_string(pos);
}
