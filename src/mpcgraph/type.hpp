/**
 * 一个初步的类型系统设计：
 *
 * Type：最抽象的类型
 *   Field
 *     ArithField: 算术域，可以认为是 Z_p，其中 p 是一个大质数
 *     BinField: 可以认为是 F_2 的扩域，域的大小是 2^k
 *   Poly：多项式类型，是一个复合类型，包含一个原子类型
 *
 * 其实有许多个 C++ 里面的 object 表示对象语言中的同一个类型还是一件不太干净的事情，所以这里我们用一种接近单例模式的方式来保证不会有这种事情发生。
 * 并且，更方便起见，这里我们干脆直接用 Type * 来表示类型好了，以一种像是 Python/Java/C# 这种更现代的语言里的方法。
 */

#pragma once

#include <unordered_map>
#include <string>

using namespace std;

class Type {
    virtual std::string to_string() {
        return "type";
    }
};

class FieldType : public Type {
    virtual std::string to_string() {
        return "field_type";
    }
};

class ArithFieldType : public FieldType {
    static ArithFieldType arith_field_type;

public:
    static ArithFieldType *get_arith_field_type() {
        return &arith_field_type;
    }

    virtual std::string to_string() {
        return "arith_field_type";
    }
};

class BinFieldType : public FieldType {
    static BinFieldType bin_field_type;

public:
    static BinFieldType *get_bin_field_type() {
        return &bin_field_type;
    }

    virtual std::string to_string() {
        return "bin_field_type";
    }
};

class PolyType : public Type {
    static unordered_map<Type *, PolyType *> polynomials;

    PolyType(Type *_elem_type) : elem_type(_elem_type) {}

public:
    Type *elem_type;

    static PolyType *get_poly_type(Type *elem_type) {
        if (polynomials.find(elem_type) == polynomials.end())
            polynomials[elem_type] = new PolyType(elem_type);
        return polynomials[elem_type];
    }

    virtual std::string to_string() {
        return "poly_type";
    }
};
