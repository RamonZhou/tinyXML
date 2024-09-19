#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <memory>
#include <cassert>
#include "bin_srl.h"
#include "xml_srl.h"
#include "type_info.h"
#include "tinyxml2.h"

/**
 * @brief the struct for testing. It contains various vals.
 */
struct A {
    int a;
    double b;
    std::string c;
    std::vector<int> d;
    std::map<std::string, int> e;
    std::set<int> f;
    char g[20];
    std::unique_ptr<int> h;
};

/**
 * @brief Set the Struct object
 * 
 * @param a 
 */
void setStruct(A &a) {
    a.a = 42;
    a.b = 3.14159;
    a.c = "Hello world!";
    for (int i = 0; i < 8; ++ i) {
        a.d.push_back(rand() % 100);
    }
    for (int i = 0; i < 8; ++ i) {
        a.e[std::to_string(i + 100)] = i;
    }
    for (int i = 0; i < 8; ++ i) {
        a.f.insert(rand() % 20 + i * 20);
    }
    for (int i = 0; i < 20; ++ i) {
        a.g[i] = 'a' + rand() % 26;
    }
    a.h = std::unique_ptr<int>(new int(10086));
}

/**
 * @brief Check if two struct object are equal
 * 
 * @param a 
 * @param b 
 */
void checkStruct(A &a, A &b) {
    std::cout << "a.a: " << a.a << " ~ " << "b.a: " << b.a << std::endl;
    std::cout << "a.b: " << a.b << " ~ " << "b.b: " << b.b << std::endl;
    std::cout << "a.c: \"" << a.c << "\" ~ " << "b.c: \"" << b.c << "\"" << std::endl;
    assert(a.a == b.a);
    assert(a.b == b.b);
    assert(a.c == b.c);
    assert(a.d.size() == b.d.size());
    for (int i = 0; i < 8; ++ i) {
        std::cout << "a.d[" << i << "]: " << a.d[i] << " ~ ";
        std::cout << "b.d[" << i << "]: " << b.d[i] << std::endl;
        assert(a.d[i] == b.d[i]);
    }
    assert(a.e.size() == b.e.size());
    for (int i = 0; i < 8; ++ i) {
        std::cout << "a.e[\"" << std::to_string(i + 100) << "\"]: " << a.e[std::to_string(i + 100)] << " ~ ";
        std::cout << "b.e[\"" << std::to_string(i + 100) << "\"]: " << b.e[std::to_string(i + 100)] << std::endl;
        assert(a.e[std::to_string(i + 100)] == b.e[std::to_string(i + 100)]);
    }
    assert(a.f.size() == b.f.size());
    for (auto it = a.f.begin(), it2 = b.f.begin(); it != a.f.end(); ++ it, ++ it2) {
        std::cout << "a.f: " << *it << " ~ ";
        std::cout << "b.f: " << *it2 << std::endl;
        assert(*it == *it2);
    }
    std::cout << "a.g: \"" << a.g << "\" ~ " << "b.g: \"" << b.g << "\"" << std::endl;
    assert(strcmp(a.g, b.g) == 0);
    std::cout << "a.h: " << *a.h << " ~ " << "b.h: " << *b.h << std::endl;
    assert(*a.h == *b.h);
}

/**
 * @brief Test for binary serialization
 */
void test_bin_srl() {
    std::cout << "Testing binary serialization" << std::endl;

    std::cout << "===========================" << std::endl;
    std::cout << "Testing int" << std::endl;
    int i = 42;
    std::cout << "Serializing int: " << i << std::endl;
    std::cout << "Serialized size: " << bin_srl::serialize(i, "test.bin") << std::endl;
    int j;
    std::cout << "Deserialized size: " << bin_srl::deserialize(j, "test.bin") << std::endl;
    std::cout << "Deserialized int: " << j << std::endl;
    assert(i == j);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing pointer" << std::endl;
    int *iptr = &i;
    std::cout << "Serializing pointer: " << *iptr << std::endl;
    std::cout << "Serialized size: " << bin_srl::serialize(iptr, "test.bin") << std::endl;
    int *jptr;
    std::cout << "Deserialized size: " << bin_srl::deserialize(jptr, "test.bin") << std::endl;
    std::cout << "Deserialized pointer: " << *jptr << std::endl;
    assert(*iptr == *jptr);
    delete jptr; // common pointers should be deleted manually to prevent memory leak

    std::cout << "===========================" << std::endl;
    std::cout << "Testing array" << std::endl;
    int arr1[6] = {3, 4, 5, 6, 7, 8};
    std::cout << "Serialized size: " << bin_srl::serialize(arr1, "test.bin") << std::endl;
    int arr2[6];
    std::cout << "Deserialized size: " << bin_srl::deserialize(arr2, "test.bin") << std::endl;
    for (int i = 0; i < 6; ++ i) {
        std::cout << "arr1[" << i << "]: " << arr1[i] << " ~ ";
        std::cout << "arr2[" << i << "]: " << arr2[i] << std::endl;
        assert(arr1[i] == arr2[i]);
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing string" << std::endl;
    std::string s = "Hello world!";
    std::cout << "Serializing string: \"" << s << "\"" << std::endl;
    std::cout << "Serialized size: " << bin_srl::serialize(s, "test.bin") << std::endl;
    std::string t = "";
    std::cout << "Deserialized size: " << bin_srl::deserialize(t, "test.bin") << std::endl;
    std::cout << "Deserialized string: \"" << t << "\"" << std::endl;
    assert(s == t);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing smart pointer" << std::endl;
    std::unique_ptr<std::string> imptr = std::unique_ptr<std::string>(new std::string("Hello world!"));
    std::cout << "Serializing pointer: \"" << *imptr << "\"" << std::endl;
    std::cout << "Serialized size: " << bin_srl::serialize(imptr, "test.bin") << std::endl;
    std::unique_ptr<std::string> jmptr;
    std::cout << "Deserialized size: " << bin_srl::deserialize(jmptr, "test.bin") << std::endl;
    std::cout << "Deserialized pointer: \"" << *jmptr << "\"" << std::endl;
    assert(*imptr == *jmptr);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing vector" << std::endl;
    std::vector<int> v;
    for (int i = 0; i < 5; ++ i) {
        v.push_back(rand() % 100);
    }
    std::cout << "Serialized size: " << bin_srl::serialize(v, "test.bin") << std::endl;
    std::vector<int> w;
    std::cout << "Deserialized size: " << bin_srl::deserialize(w, "test.bin") << std::endl;
    assert(v.size() == w.size());
    for (int i = 0; i < (int)v.size(); ++ i) {
        std::cout << "v[" << i << "]: " << v[i] << " ~ ";
        std::cout << "w[" << i << "]: " << w[i] << std::endl;
        assert(v[i] == w[i]);
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing map" << std::endl;
    std::map<std::string, int> m;
    m["un"] = 1;
    m["deux"] = 2;
    m["trois"] = 3;
    m["quatre"] = 4;
    m["cinq"] = 5;
    std::cout << "Serialized size: " << bin_srl::serialize(m, "test.bin") << std::endl;
    std::map<std::string, int> n;
    std::cout << "Deserialized size: " << bin_srl::deserialize(n, "test.bin") << std::endl;
    assert(m.size() == n.size());
    for (auto it = m.begin(); it != m.end(); ++ it) {
        std::cout << "m[\"" << it->first << "\"]: " << it->second << " ~ ";
        std::cout << "n[\"" << it->first << "\"]: " << n[it->first] << std::endl;
        assert(m[it->first] == n[it->first]);
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing set" << std::endl;
    std::set<int> s1;
    for (int i = 0; i < 7; ++ i) {
        s1.insert(rand() % 10 + i * 10);
    }
    std::cout << "Serialized size: " << bin_srl::serialize(s1, "test.bin") << std::endl;
    std::set<int> s2;
    std::cout << "Deserialized size: " << bin_srl::deserialize(s2, "test.bin") << std::endl;
    assert(s1.size() == s2.size());
    for (auto it = s1.begin(); it != s1.end(); ++ it) {
        std::cout << "s1: " << *it << " ~ ";
        std::cout << "s2: " << *it << std::endl;
        assert(s1.find(*it) != s1.end());
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing list" << std::endl;
    int larr[] = {2, 4, 6, 8, 10, 12, 14};
    std::list<int> l1(larr, larr + 7);
    std::cout << "Serialized size: " << bin_srl::serialize(l1, "test.bin") << std::endl;
    std::list<int> l2;
    std::cout << "Deserialized size: " << bin_srl::deserialize(l2, "test.bin") << std::endl;
    assert(l1.size() == l2.size());
    for (auto it = l1.begin(), it2 = l2.begin(); it != l1.end(); ++ it, ++ it2) {
        std::cout << "l1: " << *it << " ~ ";
        std::cout << "l2: " << *it2 << std::endl;
        assert((*it) == (*it2));
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing struct" << std::endl;
    A a;
    setStruct(a);
    type_info::RegisterStruct<A>("A", a, {
        {"a", a.a},
        {"b", a.b},
        {"c", a.c},
        {"d", a.d},
        {"e", a.e},
        {"f", a.f},
        {"g", a.g},
        {"h", a.h}
    });
    std::cout << "Serialized size: " << bin_srl::serialize(a, "test.bin") << std::endl;
    A b;
    std::cout << "Deserialized size: " << bin_srl::deserialize(b, "test.bin") << std::endl;
    checkStruct(a, b);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing smart pointer" << std::endl;
    std::unique_ptr<A> amptr = std::unique_ptr<A>(new A());
    setStruct(*amptr);
    std::cout << "Serialized size: " << bin_srl::serialize(amptr, "test.bin") << std::endl;
    std::unique_ptr<A> bmptr;
    std::cout << "Deserialized size: " << bin_srl::deserialize(bmptr, "test.bin") << std::endl;
    checkStruct(*amptr, *bmptr);

    std::cout << "===========================" << std::endl;
}

/**
 * @brief Test the xml serialization.
 */
void test_xml_srl() {
    std::cout << "Testing xml serialization" << std::endl;

    std::cout << "===========================" << std::endl;
    std::cout << "Testing int" << std::endl;
    int i = 42;
    std::cout << "Serializing int: " << i << std::endl;
    std::cout << "Serialized count: " << xml_srl::serialize(i, "int", "test.xml") << std::endl;
    int j;
    std::cout << "Deserialized count: " << xml_srl::deserialize(j, "int", "test.xml") << std::endl;
    std::cout << "Deserialized int: " << j << std::endl;
    assert(i == j);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing pointer" << std::endl;
    int *iptr = &i;
    std::cout << "Serializing pointer: " << *iptr << std::endl;
    std::cout << "Serialized count: " << xml_srl::serialize(iptr, "pointer", "test.xml") << std::endl;
    int *jptr;
    std::cout << "Deserialized count: " << xml_srl::deserialize(jptr, "pointer", "test.xml") << std::endl;
    std::cout << "Deserialized pointer: " << *jptr << std::endl;
    assert(*iptr == *jptr);
    delete jptr; // common pointers should be deleted manually to prevent memory leak

    std::cout << "===========================" << std::endl;
    std::cout << "Testing array" << std::endl;
    int arr1[6] = {3, 4, 5, 6, 7, 8};
    std::cout << "Serialized count: " << xml_srl::serialize(arr1, "array", "test.xml") << std::endl;
    int arr2[6];
    std::cout << "Deserialized count: " << xml_srl::deserialize(arr2, "array", "test.xml") << std::endl;
    for (int i = 0; i < 6; ++ i) {
        std::cout << "arr1[" << i << "]: " << arr1[i] << " ~ ";
        std::cout << "arr2[" << i << "]: " << arr2[i] << std::endl;
        assert(arr1[i] == arr2[i]);
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing string" << std::endl;
    std::string s = "Hello world!";
    std::cout << "Serializing string: \"" << s << "\"" << std::endl;
    std::cout << "Serialized count: " << xml_srl::serialize(s, "str", "test.xml") << std::endl;
    std::string t = "";
    std::cout << "Deserialized count: " << xml_srl::deserialize(t, "str", "test.xml") << std::endl;
    std::cout << "Deserialized string: \"" << t << "\"" << std::endl;
    assert(s == t);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing smart pointer" << std::endl;
    std::unique_ptr<std::string> imptr = std::unique_ptr<std::string>(new std::string("Hello world!"));
    std::cout << "Serializing pointer: \"" << *imptr << "\"" << std::endl;
    std::cout << "Serialized count: " << xml_srl::serialize(imptr, "uniqueptr", "test.xml") << std::endl;
    std::unique_ptr<std::string> jmptr;
    std::cout << "Deserialized count: " << xml_srl::deserialize(jmptr, "uniqueptr", "test.xml") << std::endl;
    std::cout << "Deserialized pointer: \"" << *jmptr << "\"" << std::endl;
    assert(*imptr == *jmptr);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing vector" << std::endl;
    std::vector<int> v;
    for (int i = 0; i < 5; ++ i) {
        v.push_back(rand() % 100);
    }
    std::cout << "Serialized count: " << xml_srl::serialize(v, "vector", "test.xml") << std::endl;
    std::vector<int> w;
    std::cout << "Deserialized count: " << xml_srl::deserialize(w, "vector", "test.xml") << std::endl;
    assert(v.size() == w.size());
    for (int i = 0; i < (int)v.size(); ++ i) {
        std::cout << "v[" << i << "]: " << v[i] << " ~ ";
        std::cout << "w[" << i << "]: " << w[i] << std::endl;
        assert(v[i] == w[i]);
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing map" << std::endl;
    std::map<std::string, int> m;
    m["un"] = 1;
    m["deux"] = 2;
    m["trois"] = 3;
    m["quatre"] = 4;
    m["cinq"] = 5;
    std::cout << "Serialized count: " << xml_srl::serialize(m, "map", "test.xml") << std::endl;
    std::map<std::string, int> n;
    std::cout << "Deserialized count: " << xml_srl::deserialize(n, "map", "test.xml") << std::endl;
    assert(m.size() == n.size());
    for (auto it = m.begin(); it != m.end(); ++ it) {
        std::cout << "m[\"" << it->first << "\"]: " << it->second << " ~ ";
        std::cout << "n[\"" << it->first << "\"]: " << n[it->first] << std::endl;
        assert(m[it->first] == n[it->first]);
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing set" << std::endl;
    std::set<int> s1;
    for (int i = 0; i < 7; ++ i) {
        s1.insert(rand() % 10 + i * 10);
    }
    std::cout << "Serialized count: " << xml_srl::serialize(s1, "set", "test.xml") << std::endl;
    std::set<int> s2;
    std::cout << "Deserialized count: " << xml_srl::deserialize(s2, "set", "test.xml") << std::endl;
    assert(s1.size() == s2.size());
    for (auto it = s1.begin(); it != s1.end(); ++ it) {
        std::cout << "s1: " << *it << " ~ ";
        std::cout << "s2: " << *it << std::endl;
        assert(s1.find(*it) != s1.end());
    }

    std::cout << "===========================" << std::endl;
    std::cout << "Testing struct" << std::endl;
    A a;
    setStruct(a);
    type_info::RegisterStruct<A>("A", a, {
        {"a", a.a},
        {"b", a.b},
        {"c", a.c},
        {"d", a.d},
        {"e", a.e},
        {"f", a.f},
        {"g", a.g},
        {"h", a.h}
    });
    std::cout << "Serialized count: " << xml_srl::serialize(a, "A", "test.xml") << std::endl;
    A b;
    std::cout << "Deserialized count: " << xml_srl::deserialize(b, "A", "test.xml") << std::endl;
    checkStruct(a, b);

    std::cout << "===========================" << std::endl;
    std::cout << "Testing smart pointer" << std::endl;
    std::unique_ptr<A> amptr = std::unique_ptr<A>(new A());
    setStruct(*amptr);
    std::cout << "Serialized count: " << xml_srl::serialize(amptr, "uniqueptr_struct", "test.xml") << std::endl;
    std::unique_ptr<A> bmptr;
    std::cout << "Deserialized count: " << xml_srl::deserialize(bmptr, "uniqueptr_struct", "test.xml") << std::endl;
    checkStruct(*amptr, *bmptr);

    std::cout << "===========================" << std::endl;
}

int main() {
    srand(time(0));
    test_bin_srl();
    test_xml_srl();
    return 0;
}