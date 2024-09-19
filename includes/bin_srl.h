#pragma once

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <exception>
#include <memory>
#include <cxxabi.h>
#include <type_traits>
#include "type_mtr.h"
#include "type_info.h"

/**
 * @brief The bin_srl namespace
 * This namespace contains the binary serialization functions.
 * It is used to serialize objects to binary files.
 */
namespace bin_srl {

// use macro to deal with the qualifiers
#define RR(TYPE) typename std::remove_cv<typename std::remove_reference<TYPE>::type>::type

    /**
     * @brief binary serialization output
     * @param obj
     * @param name
     * @param root
     * @return the output size of binary data
     */
    template<class T>
    unsigned int write_bin(const T& obj, std::ofstream& file) {
        if constexpr (std::is_arithmetic<RR(T)>::value) {                                                               // arithmetic type
            file.write(reinterpret_cast<const char *>(&obj), sizeof(T));
            return sizeof(T);
        } else if constexpr (std::is_pointer<RR(T)>::value || my_type_traits::is_unique_ptr<RR(T)>::value) {            // pointer type
            return write_bin(*obj, file);
        } else if constexpr (std::is_array<RR(T)>::value) {                                                             // array type
            unsigned int size = 0;
            for (int i = 0; i < (int)std::extent<RR(T)>::value; ++ i) {                                                 // for each element
                size += write_bin(obj[i], file);
            }
            return size;
        } else if constexpr (std::is_same<RR(T), std::string>::value) {                                                 // string type   
            unsigned int size = 0, len = obj.length();
            size += write_bin(static_cast<unsigned int &>(len), file);
            for (int i = 0; i < (int)len; ++ i) {
                size += write_bin(obj[i], file);
            }
            return size;
        } else if constexpr (my_type_traits::is_pair<RR(T)>::value) {                                                   // pair type
            unsigned int size = 0;
            size += write_bin(obj.first, file);
            size += write_bin(obj.second, file);
            return size;
        } else if constexpr (my_type_traits::is_container<RR(T)>::value) {                                              // container type
            unsigned int size = 0, cnt = obj.size();
            size += write_bin(static_cast<unsigned int &>(cnt), file);
            for (auto i : obj) {
                size += write_bin(i, file);
            }
            return size;
        } else if constexpr (std::is_class<RR(T)>::value) {                                                             // user defined class type
            std::string typenm = type_info::demangle(T());
            if (type_info::type_writer_bin.find(typenm) == type_info::type_writer_bin.end()) {
                std::cerr << "type " << typenm << " not registered" << std::endl;
                throw std::runtime_error("type not registered");
            }
            return type_info::type_writer_bin[typenm]((void *)&obj, file);
        } else {
            throw std::runtime_error("Unsupported type");
        }
        return 0;
    }

    /**
     * @brief binary deserialization input
     * @param obj
     * @param name
     * @param root
     * @return the input size of binary data
     */
    template<class T>
    unsigned int read_bin(T &obj, std::ifstream& file) {
        if constexpr (std::is_arithmetic<RR(T)>::value) {
            file.read(reinterpret_cast<char *>(&obj), sizeof(T));
            return sizeof(T);
        } else if constexpr (my_type_traits::is_unique_ptr<RR(T)>::value) {
            obj = std::unique_ptr<typename T::element_type>(new typename T::element_type());
            return read_bin(*obj, file);
        } else if constexpr (std::is_pointer<RR(T)>::value) {
            // memory leak!!!
            obj = new typename std::remove_pointer<T>::type();
            return read_bin(*obj, file);
        } else if constexpr (std::is_array<RR(T)>::value) {
            unsigned int size = 0;
            for (int i = 0; i < (int)std::extent<RR(T)>::value; ++ i) {
                size += read_bin(obj[i], file);
            }
            return size;
        } else if constexpr (std::is_same<RR(T), std::string>::value) {
            unsigned int size = 0, len = 0;
            size += read_bin(len, file);
            obj = "";
            for (int i = 0; i < len; ++ i) {
                char ch = 0;
                size += read_bin(ch, file);
                obj += ch;
            }
            return size;
        } else if constexpr (my_type_traits::is_pair<RR(T)>::value) {
            unsigned int size = 0;
            size += read_bin(obj.first, file);
            size += read_bin(obj.second, file);                                                         // the containers have different insert operations
            return size;                                                                                // so I distinguish them with templates
        } else if constexpr (my_type_traits::is_sequence_container<RR(T)>::value) {                     // sequence container type
            unsigned int size = 0, cnt = 0;
            size += read_bin(cnt, file);
            for (unsigned int i = 0; i < cnt; ++ i) {
                typename std::remove_cv<typename T::value_type>::type it;
                size += read_bin(it, file);
                obj.push_back(it);
            }
            return size;
        } else if constexpr (my_type_traits::is_set<RR(T)>::value) {                                    // set type
            unsigned int size = 0, cnt = 0;
            size += read_bin(cnt, file);
            for (unsigned int i = 0; i < cnt; ++ i) {
                typename std::remove_cv<typename T::value_type>::type it;
                size += read_bin(it, file);
                obj.insert(it);
            }
            return size;
        } else if constexpr (my_type_traits::is_map<RR(T)>::value) {                                    // map type
            unsigned int size = 0, cnt = 0;
            size += read_bin(cnt, file);
            for (unsigned int i = 0; i < cnt; ++ i) {
                typename std::remove_cv<typename T::key_type>::type k;
                typename std::remove_cv<typename T::mapped_type>::type v;
                size += read_bin(k, file);
                size += read_bin(v, file);
                obj.insert(std::make_pair(k, v));
            }
            return size;
        } else if constexpr (my_type_traits::is_container_adaptor<RR(T)>::value) {                      // container adaptor type
            unsigned int size = 0, cnt = 0;
            size += read_bin(cnt, file);
            for (unsigned int i = 0; i < cnt; ++ i) {
                typename std::remove_cv<typename T::value_type>::type it;
                size += read_bin(it, file);
                obj.push(it);
            }
            return size;
        } else if constexpr (std::is_class<RR(T)>::value) {
            std::string typenm = type_info::demangle(T());
            if (type_info::type_reader_bin.find(typenm) == type_info::type_reader_bin.end()) {
                std::cerr << "type " << typenm << " not registered" << std::endl;
                throw std::runtime_error("type not registered");
            }
            return type_info::type_reader_bin[typenm]((void *)&obj, file);
        } else {
            throw std::runtime_error("Unsupported type");
        }
        return 0;
    }

    /**
     * @brief binary serialization output entry function
     * @param obj
     * @param file_name
     * @return the output size of binary data
     */
    template<class T>
    unsigned int serialize(const T& obj, const char *file_name) {
        // std::string typenm = abi::__cxa_demangle(typeid(obj).name(), 0, 0, 0);
        std::ofstream file(file_name, std::ios::binary | std::ios::out);
        if (!file.is_open()) {
            std::cerr << "Error opening file" << std::endl;
            throw std::runtime_error("Error opening file");
        }
        unsigned int size = write_bin(obj, file);
        file.close();
        return size;
    }

    /**
     * @brief binary deserialization input entry function
     * @param obj
     * @param file_name
     * @return the input size of binary data
     */
    template<class T>
    unsigned int deserialize(T &obj, const char *file_name) {
        std::ifstream file(file_name, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << file_name << std::endl;
            throw std::runtime_error("Error opening file");
        }
        unsigned int size = read_bin(obj, file);
        file.close();
        return size;
    }
}