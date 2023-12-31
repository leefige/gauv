#pragma once

#include "field.hpp"
#include "party.hpp"

#include <sstream>

namespace mpc {

template<base_t BASE>
class party_missmatch: public std::exception {
    std::string msg;
public:
    party_missmatch(const par<BASE>& p1, const par<BASE>& p2) noexcept
    {
        std::stringstream ss;
        ss << "Party missmatch: '" << p1 << "' and '" << p2 << "'";
        msg = ss.str();
    }

    party_missmatch(const parset<BASE>& p1, const parset<BASE>& p2) noexcept
    {
        std::stringstream ss;
        ss << "Party missmatch: '" << p1 << "' and '" << p2 << "'";
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class argc_missmatch: public std::exception {
    std::string msg;
public:
    argc_missmatch(int want, int get) noexcept
    {
        std::stringstream ss;
        ss << "Argument number missmatch: want " << want << ", get " << get;
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class party_nonexist: public std::exception {
public:
    virtual const char* what() const noexcept override
    {
        return "Party not exist";
    }
};

template<base_t BASE>
class party_duplicated: public std::exception {
    std::string msg;
public:
    party_duplicated(const par<BASE>& old_one, const par<BASE>& new_one) noexcept
    {
        std::stringstream ss;
        ss << "Party hash code collision: " << old_one << " and " << new_one;
        msg = ss.str();
    }
    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

template<base_t BASE>
class empty_message_queue: public std::exception {
    std::string msg;
public:
    empty_message_queue(const par<BASE>& p) noexcept
    {
        std::stringstream ss;
        ss << "Message queue is empty for party" << p;
        msg = ss.str();
    }
    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

} /* namespace mpc */
