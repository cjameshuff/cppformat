// Copyright (C) 2012 Christopher James Huff
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef CPPFORMAT_H
#define CPPFORMAT_H

#include <string>
#include <iostream>
#include <list>
#include <stdexcept>
#include <stdarg.h>

// A buffer for constructing a string out of sequential fragments.
class fstring_outputbuffer {
    std::list<std::string> fragments;
    size_t length;
    
  public:
    fstring_outputbuffer(): length(0) {}
    
    // Append printf()-style formatted string
    void append_fmt(const char * fstr, ...)
    {
        std::string bfr(16, '\0');
        va_list ap;
        va_start(ap, fstr);
        int r = vsnprintf(&(bfr[0]), bfr.length(), fstr, ap);
        va_end(ap);
        if(r + 1 >= sizeof(bfr))
        {
            // Buffer was too small, try again
            bfr.resize(r + 1);// need one extra for the terminating null
            va_start(ap, fstr);
            r = vsnprintf(&(bfr[0]), r + 1, fstr, ap);
            va_end(ap);
        }
        bfr.resize(r);
        
        if(r < 0)
            throw std::invalid_argument("Format error");
        
        fragments.push_back(bfr);
        length += r;
    } // append_fmt()
    
    // Append a simple string
    void append(const std::string & str)
    {
        fragments.push_back(str);
        length += str.length();
    }
    
    // Concatenate fragments to produce output string
    std::string str() const
    {
        std::string result;
        result.reserve(length);
        for(auto & i : fragments)
            result.append(i);
        return result;
    }
}; // fstring_outputbuffer


// Format parser and output buffer state
// One of these exists for every string formatting operation, being passed between
// fstring instances behind the scenes.
class fstring_state {
    size_t fstart, fend;
    std::string format;
    fstring_outputbuffer result;
    int references;
    
  public:
    fstring_state(const std::string & fmt): fstart(0), fend(0), format(fmt), references(1) {}
    
    void retain() {++references;}
    void release() {if(--references == 0) delete this;}
    
    fstring_outputbuffer & outbuffer() {return result;}
    
    // Advance fstart, fend to location of a valid format, return format
    std::string next_fmt(const std::string & chars)
    {
        while(1)
        {
            fstart = format.find('%', fend);
            if(fstart == std::string::npos || fstart + 1 == format.length())
                throw std::invalid_argument("Format not found");
            
            // concatenate string up to format
            result.append(format.substr(fend, fstart - fend));
            
            // fend = format.find_first_not_of("0123456789+-.#hlLzjt :", fstart + 1);
            fend = format.find_first_of(chars, fstart + 1);
            if(fend == std::string::npos)
                throw std::invalid_argument("Invalid format");
            
            ++fend;
            if(format[fend - 1] == '%')
            {
                result.append("%");
            }
            else
            {
                if(chars.find(format[fend - 1]) == std::string::npos)
                    throw std::invalid_argument("Format mismatch");
                break;
            }
        }
        return format.substr(fstart, fend - fstart);
    } // next_fmt()
    
    // Append remaining fragment of format string and reset parser state
    void reset() {
        result.append(format.substr(fend));
        fstart = 0;
        fend = 0;
    }
    
    // Append remaining fragment of format string and return output string
    std::string str() {
        result.append(format.substr(fend));
        return result.str();
    }
}; // struct fstring_state


class fstring {
    mutable fstring_state * state;
    
  public:
    fstring(const std::string & fmt): state(new fstring_state(fmt)) {}
    fstring(const fstring & rhs): state(rhs.state) {state->retain();}
    ~fstring() {state->release();}
    
    void reset() const {return state->reset();}
    
    std::string next_fmt(const std::string & s) const {return state->next_fmt(s);}
    
    template<typename T>
    fstring printfmt(const std::string & fmtChars, const T & rhs) const {
        std::string fstr = state->next_fmt(fmtChars);
        state->outbuffer().append_fmt(fstr.c_str(), rhs);
        return fstring(*this);
    }
    
    fstring append(const std::string & str) const {
        state->outbuffer().append(str);
        return fstring(*this);
    }
    
    std::string str() const {return state->str();}
}; // fstring

static inline fstring operator%(const fstring & lhs, const char * rhs) {return lhs.printfmt("s", rhs);}
static inline fstring operator%(const fstring & lhs, const std::string & rhs) {return lhs.printfmt("s", rhs.c_str());}
static inline fstring operator%(const fstring & lhs, char rhs) {return lhs.printfmt("c", rhs);}
static inline fstring operator%(const fstring & lhs, int rhs) {return lhs.printfmt("di", rhs);}
static inline fstring operator%(const fstring & lhs, unsigned int rhs) {return lhs.printfmt("uxXo", rhs);}
static inline fstring operator%(const fstring & lhs, float rhs) {return lhs.printfmt("fFeEgG", rhs);}
static inline fstring operator%(const fstring & lhs, double rhs) {return lhs.printfmt("fFeEgG", rhs);}
static inline fstring operator%(const fstring & lhs, void * rhs) {return lhs.printfmt("p", rhs);}

template<typename T> static inline fstring operator%=(fstring & lhs, const T & rhs) {
    lhs % rhs;
    return lhs;
}

static inline std::ostream & operator<<(std::ostream & lhs, const fstring & rhs) {
    return lhs << rhs.str();
}

template<typename iter_t>
static inline std::string to_string(const std::string & fstr, const iter_t & begin, const iter_t & end, const std::string & sep = ", ")
{
    fstring fmt(fstr);
    auto i = begin;
    fmt %= *(i++);
    while(i != end) {
        fmt.reset();
        fmt.append(sep);
        fmt %= *(i++);
    }
    return fmt.str();
}

#endif // CPPFORMAT_H
