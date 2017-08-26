#ifndef SERIALISE_HPP_INCLUDED
#define SERIALISE_HPP_INCLUDED

#include <new>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <map>
#include <utility>
#include <type_traits>
#include <fstream>

using serialise_owner_type = int32_t;
using serialise_data_type = uint64_t;

struct serialise_data_helper
{
    static bool disk_mode;
    //static int pass;

    static std::map<serialise_owner_type, std::map<serialise_data_type, void*>> owner_to_id_to_pointer;
};

//#define serialise_owner_type int32_t
//#define serialise_data_type uint64_t

struct serialise;

struct serialisable
{
    static serialise_data_type gserialise_id;
    serialise_data_type serialise_id = gserialise_id++;

    bool owned_by_host = true;
    serialise_owner_type owner_id = 0;

    bool handled_by_client = true;

    virtual void do_serialise(serialise& s, bool ser)
    {

    }

    virtual ~serialisable(){}
};

struct serialise;

template<typename T, typename = std::enable_if_t<!std::is_base_of<serialisable, T>::value>>
inline
void lowest_add(T& v, serialise& s, std::vector<char>& data)
{
    char* pv = std::launder((char*)&v);

    for(uint32_t i=0; i<sizeof(T); i++)
    {
        data.push_back(pv[i]);
    }
}

template<typename T, typename = std::enable_if_t<!std::is_base_of<serialisable, T>::value>>
inline
void lowest_get(T& v, serialise& s, int& internal_counter, std::vector<char>& data)
{
    int prev = internal_counter;

    internal_counter += sizeof(T);

    if(internal_counter > (int)data.size())
    {
        std::cout << "Error, invalid bytefetch" << std::endl;

        v = T();
    }

    v = *std::launder((T*)&data[prev]);
}

inline
void lowest_add(serialisable& v, serialise& s, std::vector<char>& data)
{
    v.do_serialise(s, true);
}

inline
void lowest_get(serialisable& v, serialise& s, int& internal_counter, std::vector<char>& data)
{
    v.do_serialise(s, false);
}

template<typename T>
struct serialise_helper
{
    void add(T& v, serialise& s, std::vector<char>& data)
    {
        lowest_add(v, s, data);
    }

    void get(T& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        lowest_get(v, s, internal_counter, data);
    }
};

template<typename T>
struct serialise_helper<T*>
{
    void add(T* v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<serialise_owner_type> helper_owner_id;
        serialise_helper<serialise_data_type> helper1;

        if(v == nullptr)
        {
            serialise_owner_type bad_owner = -1;
            serialise_data_type bad_data = -1; ///overflows

            helper_owner_id.add(bad_owner, s, data);
            helper1.add(bad_data, s, data);

            return;
        }

        ///this is fairly expensive
        serialise_data_helper::owner_to_id_to_pointer[v->owner_id][v->serialise_id] = v;

        helper_owner_id.add(v->owner_id, s, data);
        helper1.add(v->serialise_id, s, data);

        v->do_serialise(s, true);
    }

    ///ok. So. we're specialsied on a pointer which means we're requesting a pointer
    ///but in reality when we serialise, we're going to get our 64bit id followed by the data of the pointer

    void get(T*& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<serialise_owner_type> helper_owner_id;

        serialise_owner_type owner_id;
        helper_owner_id.get(owner_id, s, internal_counter, data);

        serialise_helper<serialise_data_type> helper1;

        serialise_data_type serialise_id;
        helper1.get(serialise_id, s, internal_counter, data);

        if(owner_id == -1)
        {
            v = nullptr;
            return;
        }

        T* ptr = (T*)serialise_data_helper::owner_to_id_to_pointer[owner_id][serialise_id];

        bool was_nullptr = ptr == nullptr;

        if(was_nullptr)
        {
            ptr = new T();

            serialise_data_helper::owner_to_id_to_pointer[owner_id][serialise_id] = ptr;
        }

        serialise_helper<T> data_fetcher;
        //*ptr = data_fetcher.get(internal_counter, data);

        ptr->do_serialise(s, false);

        if(was_nullptr)
        {
            ptr->handled_by_client = false;
            ptr->owned_by_host = false;
            ptr->owner_id = owner_id;
        }

        v = ptr;
    }
};

template<typename T>
struct serialise_helper<std::vector<T>>
{
    void add(std::vector<T>& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;

        int32_t len = v.size();
        helper.add(len, s, data);

        for(uint32_t i=0; i<v.size(); i++)
        {
            serialise_helper<T> helper;
            helper.add(v[i], s, data);
        }
    }

    void get(std::vector<T>& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t length;
        helper.get(length, s, internal_counter, data);

        /*if(internal_counter + length * sizeof(T) > (int)data.size())
        {
            std::cout << "Error, invalid bytefetch" << std::endl;

            v = std::vector<T>();
        }*/

        for(int i=0; i<length; i++)
        {
            serialise_helper<T> type;

            T t;
            type.get(t, s, internal_counter, data);

            v.push_back(t);
        }
    }
};

template<>
struct serialise_helper<std::string>
{
    void add(std::string& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t len = v.size();
        helper.add(len, s, data);

        for(uint32_t i=0; i<v.size(); i++)
        {
            serialise_helper<decltype(v[i])> helper;
            helper.add(v[i], s, data);
        }
    }

    void get(std::string& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t length;
        helper.get(length, s, internal_counter, data);

        if(internal_counter + length * sizeof(char) > (int)data.size())
        {
            std::cout << "Error, invalid bytefetch" << std::endl;

            v = std::string();
        }

        for(int i=0; i<length; i++)
        {
            serialise_helper<char> type;

            char c;
            type.get(c, s, internal_counter, data);

            v.push_back(c);
        }
    }
};

#if 0
template<typename T>
struct serialise_helper<T>
{
    /*void add(const T& v, serialise& s)
    {
        v.do_serialise(s, true);
    }

    void get(const T& v, serialise& s)
    {
        v.do_serialise(s, false);
    }*/

    /*void do_method(T& v, serialise& s, bool ser)
    {
        v.do_serialise(s, ser);
    }*/

    void add(const std::string& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        helper.add((int32_t)v.size(), s, data);

        for(uint32_t i=0; i<v.size(); i++)
        {
            serialise_helper<decltype(v[i])> helper;
            helper.add(v[i], s, data);
        }
    }

    T get(serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t length = helper.get(s, internal_counter, data);

        if(internal_counter + length * sizeof(char) > (int)data.size())
        {
            std::cout << "Error, invalid bytefetch" << std::endl;

            return std::string();
        }

        std::string ret;

        for(int i=0; i<length; i++)
        {
            serialise_helper<char> type;

            ret.push_back(type.get(s, internal_counter, data));
        }

        return ret;
    }
};
#endif

struct serialise
{
    std::vector<char> data;

    int internal_counter = 0;

    template<typename T>
    void push_back(T& v)
    {
        serialise_helper<T> helper;

        helper.add(v, *this, data);
    }

    ///if pointer, look up in pointer map
    template<typename T>
    T get()
    {
        serialise_helper<T> helper;

        T val;

        helper.get(val, *this, internal_counter, data);

        return val;
    }

    template<typename T>
    void handle_serialise(T& v, bool ser)
    {
        if(ser)
        {
            push_back(v);
        }
        else
        {
            v = get<T>();
        }
    }

    /*template<typename T>
    void handle_has_method(T& v, bool ser)
    {
        serialise_helper_has_method<T> helper;

        helper.do_method(v, *this, ser);
    }*/

    bool finished_deserialising()
    {
        return internal_counter >= (int)data.size();
    }

    void save(const std::string& file)
    {
        if(data.size() == 0)
            return;

        auto myfile = std::fstream("save.game", std::ios::out | std::ios::binary);
        myfile.write((char*)&data[0], (int)data.size());
        myfile.close();
    }

    void load(const std::string& file)
    {
        internal_counter = 0;

        auto myfile = std::fstream("save.game", std::ios::in | std::ios::out | std::ios::binary);

        myfile.seekg (0, myfile.end);
        int length = myfile.tellg();
        myfile.seekg (0, myfile.beg);

        data.resize(length);

        if(length == 0)
            return;

        data.resize(length);

        myfile.read(data.data(), length);
    }
};

void test_serialisation();

#endif // SERIALISE_HPP_INCLUDED
