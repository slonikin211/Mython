#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

std::map<std::string, Token> const Lexer::keywords_tokens_ = {
    {"class"s, token_type::Class{}},
    {"return"s, token_type::Return{}},
    {"if"s, token_type::If{}},
    {"else"s, token_type::Else{}},
    {"def"s, token_type::Def{}},
    {"print"s, token_type::Print{}},
    {"and"s, token_type::And{}},
    {"or"s, token_type::Or{}},
    {"not"s, token_type::Not{}},
    {"=="s, token_type::Eq{}},
    {"!="s, token_type::NotEq{}},
    {"<="s, token_type::LessOrEq{}},
    {">="s, token_type::GreaterOrEq{}},
    {"None"s, token_type::None{}},
    {"True"s, token_type::True{}},
    {"False"s, token_type::False{}}
};

Lexer::Lexer(std::istream& input)
    :input_(input)
{
    GoToStart();
    current_token_ = NextToken();
}

const Token& Lexer::CurrentToken() const {
    return current_token_;
}

Token Lexer::NextToken() {

    if (new_line_)
    {
        static size_t spaces = 0u;
        CountSpaces(spaces);
        size_t indent_lvl = spaces / indent_space_count_;

        if (Token* token = GetIndentOrDedentToken(indent_lvl))
        {
            return *token;
        }
        spaces = 0u;
    }

    SkipSpaces();
    char peek = input_.get();

    if (isdigit(peek))  // Number
    {
        return current_token_ = GetNumberToken(peek);
    }
    else if (isalpha(peek) || peek == '_')  // Id
    {
        return current_token_ = GetIdToken(peek);
    }
    else if (peek == '\"' || peek == '\'')  // String
    {
        return current_token_ = GetStringToken(peek);
    }
    else if (peek == '#')   // Comment
    {
        SkipComment();
        return current_token_ = NextToken();
    }
    else if (peek == '\n')  // NewLine
    {
        return current_token_ = GetNewLineToken();
    }
    else if (input_.eof())  // EOF
    {
        return current_token_ = GetEofToken();
    }
    else    // Char
    {
        return current_token_ = GetCharToken(peek);
    }
}



// Support functions

void Lexer::GoToStart()
{
    while (input_.peek() == ' ' || input_.peek() == '\n' || input_.peek() == '#')
    {
        if (input_.peek() == '#')
        {
            SkipComment();
            continue;
        }
        input_.get();
    }
}

void Lexer::SkipComment()
{
    while (input_.peek() != '\n' && !input_.eof()) 
    {
        input_.get();
    }
}

void Lexer::CountSpaces(size_t& spaces)
{
    while (input_.peek() == '\n')   // skip for avoiding decreasing indent level
    {
        input_.get();
    }

    while (input_.peek() == ' ')
    {
        ++spaces;
        input_.get();

        if (input_.peek() == '\n')
        {
            spaces = 0u;
            input_.get();
        }
    }
}

void Lexer::SkipSpaces()
{
    while (input_.peek() == ' ') 
    {
        input_.get();
    }
}



// Functions for tokens

Token* Lexer::GetIndentOrDedentToken(size_t indent_level)
{
    if (indent_level == current_indent_lvl_) 
    {
        new_line_ = false;
        return nullptr; 
    }

    if (indent_level > current_indent_lvl_)
    {
        ++current_indent_lvl_;
        current_token_ = token_type::Indent{};
    }
    else
    {
        --current_indent_lvl_;
        current_token_ = token_type::Dedent{};
    }
    return &current_token_;
}

Token Lexer::GetNumberToken(char peek)
{
    empty_line_ = false;

    int number = atoi(&peek);
    while (isdigit(input_.peek()))
    {
        peek = input_.get();
        number = number * 10 + atoi(&peek);
    }
    return token_type::Number{ number };    
}

Token Lexer::GetIdToken(char peek)
{
    empty_line_ = false;

    string id;
    id += peek;

    while (std::isalpha(input_.peek()) || std::isdigit(input_.peek()) || input_.peek() == '_')
    {
        id += input_.get();
    }
    
    if (keywords_tokens_.count(id))
    {
        return keywords_tokens_.at(id);
    }
    else
    {
        return token_type::Id{ id };
    }
}

Token Lexer::GetStringToken(char peek)
{
    empty_line_ = false;

    static const map<char, char> escape_sequences{ {'n','\n'}, {'t','\t'}, {'\'','\''}, {'\"','\"'} };
    char quote = peek;  // \' or \"
    bool escaped = false;

    peek = input_.get();
    string str;
    while (!input_.eof() && !(!escaped && peek == quote))
    {
        if (!escaped)
        {
            if (peek == '\\')
            {
                escaped = true;
            }
            else
            {
                str += peek;
            }
        }
        else
        {
            if (escape_sequences.count(peek))
            {
                str += escape_sequences.at(peek);
            }
            escaped = false;
        }
        peek = input_.get();
    }
    return token_type::String{ str };
}

Token Lexer::GetNewLineToken()
{
    new_line_ = true;
    if (!empty_line_)
    {
        empty_line_ = true;
        return token_type::Newline{};
    }
    return NextToken();
}

Token Lexer::GetEofToken()
{
    if (!empty_line_)
    {
        empty_line_ = true;
        return token_type::Newline{};
    }
    return token_type::Eof{};
}

Token Lexer::GetCharToken(char peek)
{
    empty_line_ = false;

    string str;
    str += peek;
    str += input_.peek();

    if (keywords_tokens_.count(str))
    {
        input_.get();
        return keywords_tokens_.at(str);
    }
    return token_type::Char{ peek };
}

}  // namespace parse