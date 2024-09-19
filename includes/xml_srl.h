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
#include "tinyxml2.h"
#include "type_mtr.h"
#include "type_info.h"

/**
 * @brief The xml_srl namespace
 * This namespace contains the xml serialization functions.
 * It is used to serialize objects to xml files.
 */
namespace xml_srl {

// use macro to deal with the qualifiers
#define RR(TYPE) typename std::remove_cv<typename std::remove_reference<TYPE>::type>::type

    /**
     * @brief xml serialization output
     * @param obj
     * @param name
     * @param root
     * @return the output size of xml data
     */
    template<class T>
    unsigned int write_xml(const T& obj, std::string name, tinyxml2::XMLElement *root) {
        if constexpr (std::is_arithmetic<RR(T)>::value) {                                                       // arithmetic type
            tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
            elem->SetAttribute("val", std::to_string(obj).c_str());
            root->InsertEndChild(elem);
            return 1;
        } else if constexpr (std::is_pointer<RR(T)>::value || my_type_traits::is_unique_ptr<RR(T)>::value) {    // pointer type
            tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
            elem->SetAttribute("type", std::is_pointer<RR(T)>::value ? "pointer" : "unique_ptr");
            root->InsertEndChild(elem);
            return write_xml(*obj, "object", elem);
        } else if constexpr (std::is_array<RR(T)>::value) {                                                     // array type
            tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
            root->InsertEndChild(elem);
            unsigned int size = 1;
            for (int i = 0; i < (int)std::extent<RR(T)>::value; ++ i) {
                size += write_xml(obj[i], "element", elem);
            }
            return size;
        } else if constexpr (std::is_same<RR(T), std::string>::value) {                                         // string type
            tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
            tinyxml2::XMLText* text = root->GetDocument()->NewText(obj.c_str());
            elem->InsertEndChild(text);
            root->InsertEndChild(elem);
            return 1;
        } else if constexpr (my_type_traits::is_pair<RR(T)>::value) {                                           // pair type    
            unsigned int size = 1;
            tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
            root->InsertEndChild(elem);
            size += write_xml(obj.first, "first", elem);
            size += write_xml(obj.second, "second", elem);
            return size;
        } else if constexpr (my_type_traits::is_container<RR(T)>::value) {                                      // container type
            unsigned int size = 1;
            tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
            root->InsertEndChild(elem);
            for (auto i : obj) {
                size += write_xml(i, "element", elem);
            }
            return size;
        } else if constexpr (std::is_class<RR(T)>::value) {                                                     // user defined class type
            std::string typenm = type_info::demangle(T());
            if (type_info::type_writer_xml.find(typenm) == type_info::type_writer_xml.end()) {
                std::cerr << "type " << typenm << " not registered" << std::endl;
                throw std::runtime_error("type not registered");
            }
            return type_info::type_writer_xml[typenm]((void *)&obj, name, root);
        } else {
            throw std::runtime_error("Unsupported type");
        }
        return 0;
    }

    /**
     * @brief xml serialization input
     * @param obj
     * @param name
     * @param root
     * @param itself = true if the element is just itself and don't need to search among the childs
     * @return the input size of xml data
     */
    template<class T>
    unsigned int read_xml(T &obj, std::string name, tinyxml2::XMLElement *root, int itself = 0) {
        tinyxml2::XMLElement *elem = root;
        if (!itself) {
            elem = root->FirstChildElement(name.c_str());
        }
        if constexpr (std::is_arithmetic<RR(T)>::value && !std::is_floating_point<RR(T)>::value) {
            obj = std::stoi(elem->Attribute("val"));
            return 1;
        } else if constexpr (std::is_floating_point<RR(T)>::value) {
            obj = std::stod(elem->Attribute("val"));
            return 1;
        } else if constexpr (my_type_traits::is_unique_ptr<RR(T)>::value) {
            obj = std::unique_ptr<typename T::element_type>(new typename T::element_type());
            return read_xml(*obj, "object", elem);
        } else if constexpr (std::is_pointer<RR(T)>::value) {
            // memory leak!!!
            obj = new typename std::remove_pointer<T>::type();
            return read_xml(*obj, "object", elem);
        } else if constexpr (std::is_array<RR(T)>::value) {
            unsigned int size = 1;
            tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
            for (int i = 0; i < (int)std::extent<RR(T)>::value && elit != nullptr; ++ i) {
                size += read_xml(obj[i], "element", elit, 1);
                elit = elit->NextSiblingElement();
            }
            return size;
        } else if constexpr (std::is_same<RR(T), std::string>::value) {
            obj = elem->GetText();
            return 1;
        } else if constexpr (my_type_traits::is_pair<RR(T)>::value) {
            unsigned int size = 1;
            size += read_xml(obj.first, "first", elem);
            size += read_xml(obj.second, "second", elem);
            return size;
        } else if constexpr (my_type_traits::is_sequence_container<RR(T)>::value) {                     // the containers have different insert operations
            unsigned int size = 1;                                                                      // so we need to use different functions
            tinyxml2::XMLElement *elit = elem->FirstChildElement("element");                            // do the iteration in this layer
            while (elit) {                                                                              // it's not a wise design but it works
                typename std::remove_cv<typename T::value_type>::type it;
                size += read_xml(it, "element", elit, 1);
                elit = elit->NextSiblingElement();
                obj.push_back(it);
            }
            return size;
        } else if constexpr (my_type_traits::is_set<RR(T)>::value) {
            unsigned int size = 1;
            tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
            while (elit) {
                typename std::remove_cv<typename T::value_type>::type it;
                size += read_xml(it, "element", elit, 1);
                elit = elit->NextSiblingElement();
                obj.insert(it);
            }
            return size;
        } else if constexpr (my_type_traits::is_map<RR(T)>::value) {
            unsigned int size = 1;
            tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
            while (elit) {
                std::pair<typename std::remove_cv<typename T::key_type>::type,
                    typename std::remove_cv<typename T::mapped_type>::type> it;
                size += read_xml(it, "element_pair", elit, 1);
                elit = elit->NextSiblingElement();
                obj.insert(it);
            }
            return size;
        } else if constexpr (my_type_traits::is_container_adaptor<RR(T)>::value) {
            unsigned int size = 1;
            tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
            while (elit) {
                typename std::remove_cv<typename T::value_type>::type it;
                size += read_xml(it, "element", elit, 1);
                elit = elit->NextSiblingElement();
                obj.push(it);
            }
            return size;
        } else if constexpr (std::is_class<RR(T)>::value) {
            std::string typenm = type_info::demangle(T());
            if (type_info::type_reader_xml.find(typenm) == type_info::type_reader_xml.end()) {
                std::cerr << "type " << typenm << " not registered" << std::endl;
                throw std::runtime_error("type not registered");
            }
            return type_info::type_reader_xml[typenm]((void *)&obj, name, root, 0);
        } else {
            throw std::runtime_error("Unsupported type");
        }
        return 0;
    }

    /**
     * @brief xml serialization input entry point
     * @param obj
     * @param name name of the outer element
     * @param file_name
     * @return the input size of xml data
     */
    template<class T>
    unsigned int serialize(const T& obj, std::string name, const char *file_name) {
        const char * declaration = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"; // xml declaration
        tinyxml2::XMLDocument file;
        int err = file.Parse(declaration);
        tinyxml2::XMLElement* root = file.NewElement("serialization");
        file.InsertEndChild(root);
        unsigned int size = write_xml(obj, name, root);
        err = file.SaveFile(file_name);
        if (err != 0) {
            std::cerr << "Error saving xml file" << std::endl;
            throw std::runtime_error("Error saving xml file");
        }
        return size;
    }

    /**
     * @brief xml serialization output entry point
     * @param obj
     * @param name name of the outer element
     * @param file_name
     * @return the output size of xml data
     */
    template<class T>
    unsigned int deserialize(T &obj, std::string name, const char *file_name) {
        tinyxml2::XMLDocument file;
        int err = file.LoadFile(file_name);
        if (err != 0) {
            std::cerr << "Error opening xml file: " << file_name << std::endl;
            throw std::runtime_error("Error opening xml file");
        }
        tinyxml2::XMLElement* root = file.FirstChildElement("serialization");
        unsigned int size = read_xml(obj, name, root);
        err = file.SaveFile(file_name);
        return size;
    }
}