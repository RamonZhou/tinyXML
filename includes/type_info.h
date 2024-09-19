#pragma once

#include <string>
#include <cstdlib>
#include <cxxabi.h>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include "type_mtr.h"
#include "bin_srl.h"
#include "tinyxml2.h"

/**
 * @brief contains the info of different types, especially for the user defined types. also there's a tiny reflection system.
 */
namespace type_info {

// use macro to deal with the qualifiers
#define RP(TYPE) typename std::remove_cv<typename std::remove_reference<TYPE>::type>::type

    /**
     * @brief store the members of a struct. At the meantime, it's an entry point for the registry of the members.
     * @param name the name of the member
     * @param typenm the type name of the member
     * @param offset the offset of the member
     */
    struct memberPair {
        template <typename T>
        memberPair(std::string name, const T& t);
        memberPair(std::string name, std::string typenm, size_t size);
        std::string name;
        std::string typenm;
        size_t offset;
    };

    /**
     * @brief register the writer of a type
     * @param name the name of the struct
     * @param typenm the type name of the struct
     * @param members the members of the struct
     */
    struct typeInfo {
        std::string name;
        std::string typenm;
        std::vector<memberPair> members;
    };
    // the registry of the types
    std::unordered_map<std::string, typeInfo> typeInfo_map;
    std::unordered_map<std::string, std::string> type_name_map;

    // these maps store the functions we need for write and read
    std::unordered_map<std::string, std::function<unsigned int(const void*, std::ostream &)> > type_writer_bin;
    std::unordered_map<std::string, std::function<unsigned int(void*, std::istream &)> > type_reader_bin;
    std::unordered_map<std::string, std::function<unsigned int(const void*, std::string, tinyxml2::XMLElement *)> > type_writer_xml;
    std::unordered_map<std::string, std::function<unsigned int(void*, std::string, tinyxml2::XMLElement *, int itself)> > type_reader_xml;

    // demangle the type name from an object
    template <typename T>
    inline std::string demangle(const T& x) {
        int status;
        char *realname = abi::__cxa_demangle(typeid(x).name(), 0, 0, &status);
        assert(status == 0);
        std::string ret(realname);
        free(realname);
        return ret;
    }

    // demangle the type name from a typename template
    template <typename T>
    inline std::string demangle_ind(T x) {
        int status;
        char *realname = abi::__cxa_demangle(x, 0, 0, &status);
        assert(status == 0);
        std::string ret(realname);
        free(realname);
        return ret;
    }

    // get the calling name by its type name
    template <typename T>
    char *GetName(const T &x) {
        if(type_name_map.find(demangle(x)) != type_name_map.end()) {
            return const_cast<char *>(type_name_map[demangle(x)].c_str());
        } else {
            return const_cast<char *>(demangle(x).c_str());
        }
    }

    /**
     * @brief register the writer functions of a type
     */
    template <typename T>
    void RegisterWriter_(const T &x) {
        std::string typenm = type_info::demangle_ind(typeid(RP(T)).name());
        if (type_writer_bin.find(typenm) != type_writer_bin.end()) {
            return;
        }
        if constexpr (std::is_arithmetic<RP(T)>::value) {                                                       // arithmetic types
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {                                 // register binary writer
                file.write(reinterpret_cast<const char *>(obj), sizeof(T));
                return sizeof(T);
            };
            type_writer_xml[typenm] = [] (const void *obj, std::string name, tinyxml2::XMLElement *root) {      // register xml writer
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                elem->SetAttribute("val", std::to_string(*reinterpret_cast<const T *>(obj)).c_str());
                root->InsertEndChild(elem);
                return 1;
            };
        } else if constexpr (my_type_traits::is_unique_ptr<RP(T)>::value) {                                     // unique_ptr
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                const T *ptr = reinterpret_cast<const T *>(obj);
                return type_writer_bin[demangle_ind(typeid(typename T::element_type).name())](&**ptr, file);
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {       
                const T *ptr = reinterpret_cast<const T *>(obj);
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                elem->SetAttribute("type", "unique_ptr");
                root->InsertEndChild(elem);
                return type_writer_xml[demangle_ind(typeid(typename T::element_type).name())](&**ptr, "object", elem);
            };
        } else if constexpr (std::is_pointer<RP(T)>::value) {                                                   // pointer types
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                const T ptr = *reinterpret_cast<const T *>(obj);
                return type_writer_bin[demangle(*ptr)](ptr, file);
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {
                const T ptr = *reinterpret_cast<const T *>(obj);
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                elem->SetAttribute("type", "pointer");
                root->InsertEndChild(elem);
                return type_writer_xml[demangle(*ptr)](ptr, "object", elem);
            };
        } else if constexpr (std::is_array<RP(T)>::value) {                                                     // array types
            int extent = std::extent<RP(T)>::value;
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                const typename std::remove_extent<T>::type *arr = reinterpret_cast<const typename std::remove_extent<T>::type *>(obj);
                for (size_t i = 0; i < std::extent<RP(T)>::value; i++) {
                    if (type_writer_bin.find(demangle(arr[i])) == type_writer_bin.end()) {
                        std::cerr << "No writer for " << demangle(arr[i]) << std::endl;
                    }
                    type_writer_bin[demangle(arr[i])](&arr[i], file);
                }
                return sizeof(T);
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {       
                const typename std::remove_extent<T>::type *arr = reinterpret_cast<const typename std::remove_extent<T>::type *>(obj);
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                root->InsertEndChild(elem);
                unsigned int size = 1;
                for (size_t i = 0; i < std::extent<RP(T)>::value; i++) {
                    size += type_writer_xml[demangle(arr[i])](&arr[i], "element", elem);
                }
                return size;
            };
        } else if constexpr (std::is_same<RP(T), std::string>::value) {                                         // string types
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                const T *str = reinterpret_cast<const T *>(obj);
                unsigned int size = str->size();
                file.write(reinterpret_cast<const char *>(&size), sizeof(unsigned int));
                file.write(str->c_str(), str->size());
                return str->size() + sizeof(unsigned int);
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {
                const T *str = reinterpret_cast<const T *>(obj);
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                tinyxml2::XMLText *text = root->GetDocument()->NewText(str->c_str());
                elem->InsertEndChild(text);
                root->InsertEndChild(elem);
                return 1;
            };
        } else if constexpr (my_type_traits::is_pair<RP(T)>::value) {                                           // pair types
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                const T *pair = reinterpret_cast<const T *>(obj);
                type_writer_bin[demangle(pair->first)](&pair->first, file);
                type_writer_bin[demangle(pair->second)](&pair->second, file);
                return sizeof(T);
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {
                const T *pair = reinterpret_cast<const T *>(obj);
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                root->InsertEndChild(elem);
                unsigned int size = 1;
                size += type_writer_xml[demangle(pair->first)](&pair->first, "first", elem);
                size += type_writer_xml[demangle(pair->second)](&pair->second, "second", elem);
                return size;
            };
        } else if constexpr (my_type_traits::is_container<RP(T)>::value && !my_type_traits::is_map<RP(T)>::value) { // container types (not map)
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                const T *cont = reinterpret_cast<const T *>(obj);
                unsigned int size = 1, cnt = cont->size();
                file.write(reinterpret_cast<const char *>(&cnt), sizeof(unsigned int));
                for (auto it = cont->begin(); it != cont->end(); it++) {
                    size += type_writer_bin[demangle(*it)]((void *)(&(*it)), file);
                }
                return size;
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {
                const T *cont = reinterpret_cast<const T *>(obj);
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                root->InsertEndChild(elem);
                unsigned int size = 1;
                for (auto it = cont->begin(); it != cont->end(); it++) {
                    size += type_writer_xml[demangle(*it)]((void *)(&(*it)), "element", elem);
                }
                return size;
            };
        } else if constexpr (my_type_traits::is_map<RP(T)>::value) {                                                // map types
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                const T *cont = reinterpret_cast<const T *>(obj);
                unsigned int size = (unsigned int)sizeof(unsigned int), cnt = cont->size();
                file.write(reinterpret_cast<const char *>(&cnt), sizeof(unsigned int));
                for (auto it = cont->begin(); it != cont->end(); it++) {
                    std::pair<typename T::key_type, typename T::mapped_type> pair = *it;
                    size += type_writer_bin[demangle(pair)]((void *)(&pair), file);
                }
                return size;
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {
                const T *cont = reinterpret_cast<const T *>(obj);
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                root->InsertEndChild(elem);
                unsigned int size = 1;
                for (auto it = cont->begin(); it != cont->end(); it++) {
                    std::pair<typename T::key_type, typename T::mapped_type> pair = *it;
                    size += type_writer_xml[demangle(pair)]((void *)(&pair), "element_pair", elem);
                }
                return size;
            };
        } else if constexpr (std::is_class<RP(T)>::value) {                                                         // user defined class types
            type_writer_bin[typenm] = [](const void *obj, std::ostream &file) {
                unsigned int size = 0;
                typeInfo &info = typeInfo_map[demangle_ind(typeid(RP(T)).name())];
                for (auto i = info.members.begin(); i != info.members.end(); i++) {
                    const char *dat = reinterpret_cast<const char *>(obj) + i->offset;
                    size += type_writer_bin[i->typenm]((void *)dat, file);
                }
                return size;
            };
            type_writer_xml[typenm] = [](const void *obj, std::string name, tinyxml2::XMLElement *root) {
                typeInfo &info = typeInfo_map[demangle_ind(typeid(RP(T)).name())];
                tinyxml2::XMLElement *elem = root->GetDocument()->NewElement(name.c_str());
                elem->SetAttribute("type", GetName(*reinterpret_cast<const T*>(obj)));
                root->InsertEndChild(elem);
                unsigned int size = 1;
                for (auto i = info.members.begin(); i != info.members.end(); i++) {
                    const char *dat = reinterpret_cast<const char *>(obj) + i->offset;
                    size += type_writer_xml[i->typenm]((void *)dat, i->name, elem);
                }
                return size;
            };
        } else {
            throw std::runtime_error("Unsupported type");
        }
    }

    /**
     * @brief register the reader functions
     */
    template <typename T>
    void RegisterReader_(const T &x) {
        std::string typenm = type_info::demangle_ind(typeid(RP(T)).name());
        if (type_reader_bin.find(typenm) != type_reader_bin.end()) {
            return;
        }
        if constexpr (std::is_arithmetic<RP(T)>::value) {
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                file.read(reinterpret_cast<char *>(obj), sizeof(T));
                return sizeof(T);
            };
            if constexpr (std::is_floating_point<RP(T)>::value) {
                type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                    tinyxml2::XMLElement *elem = root;
                    if (!itself) elem = elem->FirstChildElement(name.c_str());
                    T *dat = reinterpret_cast<T *>(obj);
                    *dat = std::stod(elem->Attribute("val"));
                    return 1;
                };
            } else {
                type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                    tinyxml2::XMLElement *elem = root;
                    if (!itself) elem = elem->FirstChildElement(name.c_str());
                    T *dat = reinterpret_cast<T *>(obj);
                    *dat = std::stoi(elem->Attribute("val"));
                    return 1;
                };
            }
        } else if constexpr (my_type_traits::is_unique_ptr<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                T *ptr = reinterpret_cast<T *>(obj);
                *ptr = std::unique_ptr<typename T::element_type>(new typename T::element_type());
                return type_reader_bin[demangle_ind(typeid(typename T::element_type).name())](&**ptr, file);
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                T *ptr = reinterpret_cast<T *>(obj);
                *ptr = std::unique_ptr<typename T::element_type>(new typename T::element_type());
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                return type_reader_xml[demangle_ind(typeid(typename T::element_type).name())](&**ptr, "object", elem, 0);
            };
        } else if constexpr (std::is_pointer<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                obj = new typename std::remove_pointer<T>::type();
                return type_reader_bin[demangle_ind(typeid(std::remove_pointer<T>::type).name())](obj, file);
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                obj = new typename std::remove_pointer<T>::type();
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                return type_reader_xml[demangle_ind(typeid(std::remove_pointer<T>::type).name())](obj, "object", elem, 0);
            };
        } else if constexpr (std::is_array<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                typename std::remove_extent<T>::type *arr = reinterpret_cast<typename std::remove_extent<T>::type *>(obj);
                for (size_t i = 0; i < std::extent<RP(T)>::value; i++) {
                    if (type_reader_bin.find(demangle(arr[i])) == type_reader_bin.end()) {
                        std::cerr << "No reader for " << demangle(arr[i]) << std::endl;
                    }
                    type_reader_bin[demangle(arr[i])](&arr[i], file);
                }
                return sizeof(T);
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                typename std::remove_extent<T>::type *arr = reinterpret_cast<typename std::remove_extent<T>::type *>(obj);
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
                unsigned int size = 1;
                for (size_t i = 0; i < std::extent<RP(T)>::value, elit != nullptr; i++) {
                    size += type_reader_xml[demangle(arr[i])](&arr[i], "element", elit, 1);
                    elit = elit->NextSiblingElement();
                }
                return size;
            };
        } else if constexpr (std::is_same<RP(T), std::string>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                T *str = reinterpret_cast<T *>(obj);
                unsigned int size = str->size();
                str->clear();
                file.read(reinterpret_cast<char *>(&size), sizeof(unsigned int));
                for (int i = 0; i < (int)size; ++ i) {
                    char c;
                    file.read(&c, 1);
                    str->push_back(c);
                }
                return size + sizeof(unsigned int);
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                T *str = reinterpret_cast<T *>(obj);
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                *str = elem->GetText();
                return 1;
            };
        } else if constexpr (my_type_traits::is_pair<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                T *pair = reinterpret_cast<T *>(obj);
                type_reader_bin[demangle(pair->first)](&pair->first, file);
                type_reader_bin[demangle(pair->second)](&pair->second, file);
                return sizeof(T);
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                T *pair = reinterpret_cast<T *>(obj);
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                unsigned int size = 1;
                size += type_reader_xml[demangle(pair->first)](&pair->first, "first", elem, 0);
                size += type_reader_xml[demangle(pair->second)](&pair->second, "second", elem, 0);
                return size;
            };
        } else if constexpr (my_type_traits::is_sequence_container<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                T *cont = reinterpret_cast<T *>(obj);
                unsigned int size = 1, cnt = 0;
                file.read(reinterpret_cast<char *>(&cnt), sizeof(unsigned int));
                size = sizeof(unsigned int);
                for (int i = 0; i < (int)cnt; ++ i) {
                    typename std::remove_cv<typename T::value_type>::type elem;
                    size += type_reader_bin[demangle(elem)]((void *)(&elem), file);
                    cont->push_back(elem);
                }
                return size;
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                T *cont = reinterpret_cast<T *>(obj);
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                int size = 1;
                tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
                while(elit) {
                    typename std::remove_cv<typename T::value_type>::type it;
                    size += type_reader_xml[demangle(it)](&it, "element", elit, 1);
                    cont->push_back(it);
                    elit = elit->NextSiblingElement();
                }
                return size;
            };
        } else if constexpr (my_type_traits::is_container_adaptor<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                T *cont = reinterpret_cast<T *>(obj);
                unsigned int size = 0, cnt = 0;
                file.read(reinterpret_cast<char *>(&cnt), sizeof(unsigned int));
                size = sizeof(unsigned int);
                for (int i = 0; i < (int)cnt; ++ i) {
                    typename std::remove_cv<typename T::value_type>::type elem;
                    size += type_reader_bin[demangle(elem)]((void *)(&elem), file);
                    cont->push(elem);
                }
                return size;
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                T *cont = reinterpret_cast<T *>(obj);
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                int size = 1;
                tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
                while(elit) {
                    typename std::remove_cv<typename T::value_type>::type it;
                    size += type_reader_xml[demangle(it)](&it, "element", elit, 1);
                    cont->push(it);
                    elit = elit->NextSiblingElement();
                }
                return size;
            };
        } else if constexpr (my_type_traits::is_set<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                T *cont = reinterpret_cast<T *>(obj);
                unsigned int size = 0, cnt = 0;
                file.read(reinterpret_cast<char *>(&cnt), sizeof(unsigned int));
                size = sizeof(unsigned int);
                for (int i = 0; i < (int)cnt; ++ i) {
                    typename std::remove_cv<typename T::value_type>::type elem;
                    size += type_reader_bin[demangle(elem)]((void *)(&elem), file);
                    cont->insert(elem);
                }
                return size;
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                T *cont = reinterpret_cast<T *>(obj);
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                int size = 1;
                tinyxml2::XMLElement *elit = elem->FirstChildElement("element");
                while(elit) {
                    typename std::remove_cv<typename T::value_type>::type it;
                    size += type_reader_xml[demangle(it)](&it, "element", elit, 1);
                    cont->insert(it);
                    elit = elit->NextSiblingElement();
                }
                return size;
            };
        } else if constexpr (my_type_traits::is_map<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                T *cont = reinterpret_cast<T *>(obj);
                unsigned int size = 0, cnt = 0;
                file.read(reinterpret_cast<char *>(&cnt), sizeof(unsigned int));
                size = sizeof(unsigned int);
                for (int i = 0; i < (int)cnt; ++ i) {
                    std::pair<typename std::remove_cv<typename T::key_type>::type,
                        typename std::remove_cv<typename T::mapped_type>::type> elem;
                    size += type_reader_bin[demangle(elem)]((void *)(&elem), file);
                    cont->insert(elem);
                }
                return size;
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                T *cont = reinterpret_cast<T *>(obj);
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                int size = 1;
                tinyxml2::XMLElement *elit = elem->FirstChildElement("element_pair");
                while(elit) {
                    std::pair<typename std::remove_cv<typename T::key_type>::type,
                        typename std::remove_cv<typename T::mapped_type>::type> it;
                    size += type_reader_xml[demangle(it)](&it, "element_pair", elit, 1);
                    cont->insert(it);
                    elit = elit->NextSiblingElement();
                }
                return size;
            };
        } else if constexpr (std::is_class<RP(T)>::value) {
            // bin
            type_reader_bin[typenm] = [](void *obj, std::istream &file) {
                unsigned int size = 0;
                typeInfo &info = typeInfo_map[demangle_ind(typeid(RP(T)).name())];
                for (auto i = info.members.begin(); i != info.members.end(); i++) {
                    char *dat = reinterpret_cast<char *>(obj) + i->offset;
                    size += type_reader_bin[i->typenm]((void *)dat, file);
                }
                return size;
            };
            // xml
            type_reader_xml[typenm] = [](void *obj, std::string name, tinyxml2::XMLElement *root, int itself) {
                tinyxml2::XMLElement *elem = root;
                if (!itself) elem = elem->FirstChildElement(name.c_str());
                unsigned int size = 1;
                typeInfo &info = typeInfo_map[demangle_ind(typeid(RP(T)).name())];
                for (auto i = info.members.begin(); i != info.members.end(); i++) {
                    char *dat = reinterpret_cast<char *>(obj) + i->offset;
                    size += type_reader_xml[i->typenm]((void *)dat, i->name, elem, 0);
                }
                return size;
            };
        } else {
            throw std::runtime_error("Unsupported type");
        }
    }

    // overall registry of a type
    template <typename T>
    void RegisterBaseType_(const T &x) {
        RegisterWriter_(x);     // register writer
        RegisterReader_(x);     // register reader
        if (type_name_map.find(demangle(x)) == type_name_map.end()) {   // register type name
            type_name_map[demangle(x)] = demangle(x);
        }
    }

    // register a type recursively
    template <typename T>
    void RegisterType_(const T &t) {
        std::string typenm = demangle(t);
        if (type_writer_bin.find(typenm) != type_writer_bin.end()) return;
        if constexpr (std::is_arithmetic<RP(T)>::value ||
                      std::is_same<RP(T), std::string>::value) {
            RegisterBaseType_(t);
        } else {
            if constexpr (std::is_pointer<RP(T)>::value) {
                typename std::remove_pointer<RP(T)>::type tmp;
                RegisterType_(tmp);
                RegisterBaseType_(t);
            } else if constexpr (my_type_traits::is_unique_ptr<RP(T)>::value) {
                typename T::element_type tmp;
                RegisterType_(tmp);
                RegisterBaseType_(t);
            } else if constexpr (std::is_array<RP(T)>::value) {
                typename std::remove_extent<RP(T)>::type tmp;
                RegisterType_(tmp);
                RegisterBaseType_(t);
            } else if constexpr (my_type_traits::is_pair<RP(T)>::value) {
                RegisterType_(t.first);
                RegisterType_(t.second);
                RegisterBaseType_(t);
            } else if constexpr (my_type_traits::is_container<RP(T)>::value && !my_type_traits::is_map<RP(T)>::value) {
                typename std::remove_cv<typename T::value_type>::type tmp;
                RegisterType_(tmp);
                RegisterBaseType_(t);
            } else if constexpr(my_type_traits::is_map<RP(T)>::value) {
                typename std::pair<typename std::remove_cv<typename T::key_type>::type,
                    typename std::remove_cv<typename T::mapped_type>::type> tmp;
                RegisterType_(tmp);
                RegisterBaseType_(t);
            } else {
                std::cerr << "Unsupported or unregistered type: " << typenm << std::endl;
                throw std::runtime_error("Unsupported or unregistered type");
            }
        }
    }

    // register a member type
    template <typename T>
    memberPair::memberPair(std::string name, const T& t): name(name), offset((size_t)&t) {
        RegisterType_(t);
        this->typenm = demangle(t);
    }

    // the common ctor of memberPair
    memberPair::memberPair(std::string name, std::string typenm, size_t size): name(name), typenm(typenm), offset(size) {}

    // the register function of a struct. This is the function for user to call
    template <typename T>
    void RegisterStruct(std::string name, const T &x, std::initializer_list<memberPair> members) {
        if (typeInfo_map.find(name) != typeInfo_map.end()) return;
        typeInfo newType;   // register new struct type
        newType.name = name;
        newType.typenm = demangle(x);
        newType.members.clear();
        for (auto& member: members) {  // get all the members
            newType.members.push_back(memberPair(member.name, member.typenm, member.offset - (size_t)&x));
        }
        typeInfo_map[newType.typenm] = newType;
        if (type_name_map.find(demangle(x)) == type_name_map.end()) {
            type_name_map[demangle(x)] = name;
        }
        RegisterBaseType_(x);  // register writer and reader
    }
}