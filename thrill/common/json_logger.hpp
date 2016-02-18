/*******************************************************************************
 * thrill/common/json_logger.hpp
 *
 * Logger for statistical output in JSON format for post-processing.
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2016 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#pragma once
#ifndef THRILL_COMMON_JSON_LOGGER_HEADER
#define THRILL_COMMON_JSON_LOGGER_HEADER

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace thrill {
namespace common {

// forward declarations
class JsonLine;
template <typename Type>
struct JsonLinePutSwitch;

/*!
 * JsonLogger is a receiver of JSON output objects for logging.
 */
class JsonLogger
{
public:
    JsonLogger() { }

    //! create new JsonLine instance which will be written to this logger.
    JsonLine line();

    template <typename Type>
    JsonLine operator << (const Type& t);

    //! method called by output objects
    void Output() {
        std::cout << '{' << oss_.str() << '}' << std::endl;
    }

public:
    //! collector stream
    std::ostringstream oss_;

    //! elements counter
    size_t elements_ = 0;

    //! friends for sending to oss_
    friend class JsonLine;

    template <typename Type>
    friend struct JsonLinePutSwitch;
};

/*!
 * JsonLine is an object used to aggregate a set of key:value pairs for output
 * into a JSON log.
 */
class JsonLine
{
public:
    //! when destructed this object is delivered to the output.
    JsonLogger& logger_;

    //! ctor: bind output
    explicit JsonLine(JsonLogger& logger)
        : logger_(logger) { }

    // //! ctor: initialize with a list of key:value pairs of variadic type.
    // template <typename ... Args>
    // explicit JsonLine(const Args& ... args) {
    //     using ForeachExpander = int[];
    //     (void)ForeachExpander { (operator << (args), 0) ... };
    // }

    //! non-copyable: delete copy-constructor
    JsonLine(const JsonLine&) = delete;
    //! non-copyable: delete assignment operator
    JsonLine& operator = (const JsonLine&) = delete;
    //! move-constructor: default
    JsonLine(JsonLine&&) = default;
    //! move-assignment operator: default
    JsonLine& operator = (JsonLine&&) = default;

    //! output any type
    template <typename Type>
    JsonLine& operator << (Type const& t);

    //! output any type, decay array types to pointers
    template <typename Type>
    JsonLine & PutDecay(Type const& t);

    //! destructor: deliver to output
    ~JsonLine() {
        assert(logger_.elements_ % 2 == 0);
        logger_.Output();
    }

    //! put an elements separator (either ',' or ':') and increment counter.
    void PutSeparator() {
        if (logger_.elements_ > 0) {
            logger_.oss_ << (logger_.elements_ % 2 == 0 ? ',' : ':');
        }
        logger_.elements_++;
    }

    void PutEscapedChar(char ch) {
        // from: http://stackoverflow.com/a/7725289
        switch (ch) {
        case '\\': logger_.oss_ << '\\' << '\\';
            break;
        case '"': logger_.oss_ << '\\' << '"';
            break;
        case '/': logger_.oss_ << '\\' << '/';
            break;
        case '\b': logger_.oss_ << '\\' << 'b';
            break;
        case '\f': logger_.oss_ << '\\' << 'f';
            break;
        case '\n': logger_.oss_ << '\\' << 'n';
            break;
        case '\r': logger_.oss_ << '\\' << 'r';
            break;
        case '\t': logger_.oss_ << '\\' << 't';
            break;
        default: logger_.oss_ << ch;
            break;
        }
    }
};

/******************************************************************************/
// Template specializations for JsonLine

static inline
JsonLine & Put(JsonLine& line, bool const& value) {
    line.logger_.oss_ << (value ? "true" : "false");
    return line;
}

static inline
JsonLine & Put(JsonLine& line, int const& value) {
    line.logger_.oss_ << value;
    return line;
}

static inline
JsonLine & Put(JsonLine& line, unsigned int const& value) {
    line.logger_.oss_ << value;
    return line;
}

static inline
JsonLine & Put(JsonLine& line, long const& value) {
    line.logger_.oss_ << value;
    return line;
}

static inline
JsonLine & Put(JsonLine& line, unsigned long const& value) {
    line.logger_.oss_ << value;
    return line;
}

static inline
JsonLine & Put(JsonLine& line, double const& value) {
    line.logger_.oss_ << value;
    return line;
}

static inline
JsonLine & Put(JsonLine& line, const char* const& str) {
    line.logger_.oss_ << '"';
    for (const char* s = str; *s; ++s) line.PutEscapedChar(*s);
    line.logger_.oss_ << '"';
    return line;
}

static inline
JsonLine & Put(JsonLine& line, std::string const& str) {
    line.logger_.oss_ << '"';
    for (std::string::const_iterator i = str.begin(); i != str.end(); ++i)
        line.PutEscapedChar(*i);
    line.logger_.oss_ << '"';
    return line;
}

template <typename Type>
static inline
JsonLine & Put(JsonLine& line, std::vector<Type> const& vec) {
    line.logger_.oss_ << '[';
    for (typename std::vector<Type>::const_iterator it = vec.begin();
         it != vec.end(); ++it) {
        if (it != vec.begin())
            line.logger_.oss_ << ',';
        line.PutDecay(*it);
    }
    line.logger_.oss_ << ']';
    return line;
}

// due to problems with outputting const char[N], borrowed from
// http://stackoverflow.com/questions/6559622/template-specialization-for...

template <typename Type>
struct ArrayToPointerDecay
{
    using type = Type;
};

template <typename Type, std::size_t N>
struct ArrayToPointerDecay<Type[N]>
{
    using type = const Type *;
};

template <typename Type>
inline JsonLine& JsonLine::PutDecay(const Type& t) {
    using Decayed = typename ArrayToPointerDecay<Type>::type;
    return Put(*this, static_cast<Decayed>(t));
}

template <typename Type>
inline JsonLine& JsonLine::operator << (const Type& t) {
    PutSeparator();
    return PutDecay(t);
}

/******************************************************************************/
// JsonLogger

inline JsonLine JsonLogger::line() {
    oss_.str("");
    elements_ = 0;

    JsonLine out(*this);
    out << "ts"
        << std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return out;
}

template <typename Type>
inline JsonLine JsonLogger::operator << (const Type& t) {
    JsonLine out = line();
    out << t;
    return out;
}

} // namespace common
} // namespace thrill

#endif // !THRILL_COMMON_JSON_LOGGER_HEADER

/******************************************************************************/
