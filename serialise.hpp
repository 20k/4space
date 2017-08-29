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
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

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

    ///overload operator = ?
    ///If we duplicate something we don't own, REALLY BAD STUFF will happen
    void get_new_serialise_id()
    {
        serialise_id = gserialise_id++;
    }

    bool owned_by_host = true;
    serialise_owner_type owner_id = 0;

    bool handled_by_client = true;

    virtual void do_serialise(serialise& s, bool ser)
    {

    }

    virtual ~serialisable(){}
};

struct serialise;

/*template<typename T, typename = std::enable_if_t<!std::is_base_of<serialisable, T>::value && std::is_arithmetic<typename std::remove_reference<T>::type>::type>>
inline
void lowest_add(T& v, serialise& s, std::vector<char>& data)
{
    char* pv = std::launder((char*)&v);

    for(uint32_t i=0; i<sizeof(T); i++)
    {
        data.push_back(pv[i]);
    }
}*/

template<typename T, typename = std::enable_if_t<!std::is_base_of_v<serialisable, T> && std::is_standard_layout_v<std::remove_reference_t<T>>>>
inline
void lowest_add(T& v, serialise& s, std::vector<char>& data)
{
    char* pv = std::launder((char*)&v);

    for(uint32_t i=0; i<sizeof(T); i++)
    {
        data.push_back(pv[i]);
    }
}

template<typename T, typename = std::enable_if_t<!std::is_base_of_v<serialisable, T> && std::is_standard_layout_v<std::remove_reference_t<T>>>>
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

/*template<typename T, typename = std::enable_if_t<!std::is_base_of<serialisable, T>::value>>
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
}*/



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

        ///we need to clear this every disk save atm
        auto last_ptr = serialise_data_helper::owner_to_id_to_pointer[v->owner_id][v->serialise_id];

        ///this is fairly expensive
        serialise_data_helper::owner_to_id_to_pointer[v->owner_id][v->serialise_id] = v;

        helper_owner_id.add(v->owner_id, s, data);
        helper1.add(v->serialise_id, s, data);

        ///we're writing out this element for the first time
        if(last_ptr == nullptr)
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

            //std::cout << typeid(T).name() << " " << ptr << std::endl;

            serialise_data_helper::owner_to_id_to_pointer[owner_id][serialise_id] = ptr;
        }

        //serialise_helper<T> data_fetcher;
        //*ptr = data_fetcher.get(internal_counter, data);


        if(was_nullptr)
        {
            ptr->handled_by_client = false;
            ptr->owned_by_host = false;
            ptr->owner_id = owner_id;

            ///we're reading this element for the first time
            ptr->do_serialise(s, false);
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

        for(int i=0; i<length; i++)
        {
            serialise_helper<T> type;

            T t;
            type.get(t, s, internal_counter, data);

            v.push_back(t);
        }
    }
};

template<typename T, typename U>
struct serialise_helper<std::map<T, U>>
{
    void add(std::map<T, U>& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;

        int32_t len = v.size();
        helper.add(len, s, data);

        for(auto& i : v)
        {
            serialise_helper<T> h1;
            serialise_helper<U> h2;

            T f_id = i.first;

            h1.add(f_id, s, data);
            h2.add(i.second, s, data);
        }
    }

    void get(std::map<T, U>& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t length;
        helper.get(length, s, internal_counter, data);

        for(int i=0; i<length; i++)
        {
            T first;
            U second;

            serialise_helper<T> h1;
            serialise_helper<U> h2;

            h1.get(first, s, internal_counter, data);
            h2.get(second, s, internal_counter, data);

            v[first] = second;
        }
    }
};

template<typename T, typename U>
struct serialise_helper<std::unordered_map<T, U>>
{
    void add(std::unordered_map<T, U>& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;

        int32_t len = v.size();
        helper.add(len, s, data);

        for(auto& i : v)
        {
            serialise_helper<T> h1;
            serialise_helper<U> h2;

            T f_id = i.first;

            h1.add(f_id, s, data);
            h2.add(i.second, s, data);
        }
    }

    void get(std::unordered_map<T, U>& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t length;
        helper.get(length, s, internal_counter, data);

        for(int i=0; i<length; i++)
        {
            T first;
            U second;

            serialise_helper<T> h1;
            serialise_helper<U> h2;

            h1.get(first, s, internal_counter, data);
            h2.get(second, s, internal_counter, data);

            v[first] = second;
        }
    }
};

template<typename T>
struct serialise_helper<std::set<T>>
{
    void add(std::set<T>& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;

        int32_t len = v.size();
        helper.add(len, s, data);

        for(auto& i : v)
        {
            auto elem = i;

            serialise_helper<T> helper;
            helper.add(elem, s, data);
        }
    }

    void get(std::set<T>& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t length;
        helper.get(length, s, internal_counter, data);

        for(int i=0; i<length; i++)
        {
            serialise_helper<T> type;

            T t;
            type.get(t, s, internal_counter, data);

            v.insert(t);
        }
    }
};

template<typename T>
struct serialise_helper<std::unordered_set<T>>
{
    void add(std::unordered_set<T>& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;

        int32_t len = v.size();
        helper.add(len, s, data);

        for(auto& i : v)
        {
            auto elem = i;

            serialise_helper<T> helper;
            helper.add(elem, s, data);
        }
    }

    void get(std::unordered_set<T>& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<int32_t> helper;
        int32_t length;
        helper.get(length, s, internal_counter, data);

        for(int i=0; i<length; i++)
        {
            serialise_helper<T> type;

            T t;
            type.get(t, s, internal_counter, data);

            v.insert(t);
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

            return;
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

template<>
struct serialise_helper<sf::Texture>
{
    void add(sf::Texture& v, serialise& s, std::vector<char>& data)
    {
        serialise_helper<vec2i> helper;
        vec2i dim = {v.getSize().x, v.getSize().y};
        helper.add(dim, s, data);

        serialise_helper<char> smooth;
        char is_smooth = (char)v.isSmooth();
        smooth.add(is_smooth, s, data);

        /*for(uint32_t i=0; i<v.size(); i++)
        {
            serialise_helper<decltype(v[i])> helper;
            helper.add(v[i], s, data);
        }*/

        sf::Image img = v.copyToImage();

        for(int y=0; y<dim.y(); y++)
        {
            for(int x=0; x<dim.x(); x++)
            {
                sf::Color col = img.getPixel(x, y);

                serialise_helper<uint8_t> h1;
                serialise_helper<uint8_t> h2;
                serialise_helper<uint8_t> h3;
                serialise_helper<uint8_t> h4;

                h1.add(col.r, s, data);
                h2.add(col.g, s, data);
                h3.add(col.b, s, data);
                h4.add(col.a, s, data);
            }
        }
    }

    void get(sf::Texture& v, serialise& s, int& internal_counter, std::vector<char>& data)
    {
        /*serialise_helper<int32_t> helper;
        int32_t length;
        helper.get(length, s, internal_counter, data);

        if(internal_counter + length * sizeof(char) > (int)data.size())
        {
            std::cout << "Error, invalid bytefetch" << std::endl;

            v = std::string();

            return;
        }

        for(int i=0; i<length; i++)
        {
            serialise_helper<char> type;

            char c;
            type.get(c, s, internal_counter, data);

            v.push_back(c);
        }*/

        serialise_helper<vec2i> helper;

        vec2i dim;
        helper.get(dim,s, internal_counter, data);

        serialise_helper<char> smooth_helper;
        char smooth;
        smooth_helper.get(smooth, s, internal_counter, data);

        sf::Image img;
        img.create(dim.x(), dim.y(), (sf::Uint8*)&data[internal_counter]);

        internal_counter += dim.x() * dim.y() * sizeof(sf::Uint8) * 4;

        v.loadFromImage(img);

        v.setSmooth((bool)smooth);
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

///at the moment serialisation will cause essentially a chain reaction and serialise anything related to an object
///ie pretty much everything
///this is great for disk mode, but in network mode I need to make sure that it never serialises references unless
///we actually need to
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

    bool finished_deserialising()
    {
        return internal_counter >= (int)data.size();
    }

    ///need to use differetn serialise_data_helper::owner_to_id_to_pointer.clear() strategy for network
    ///and disk mode. Ie two separate tables
    void save(const std::string& file)
    {
        serialise_data_helper::owner_to_id_to_pointer.clear();

        if(data.size() == 0)
            return;

        auto myfile = std::fstream("save.game", std::ios::out | std::ios::binary);
        myfile.write((char*)&data[0], (int)data.size());
        myfile.close();
    }

    void load(const std::string& file)
    {
        serialise_data_helper::owner_to_id_to_pointer.clear();

        internal_counter = 0;

        auto myfile = std::fstream("save.game", std::ios::in | std::ios::out | std::ios::binary);

        myfile.seekg (0, myfile.end);
        int length = myfile.tellg();
        myfile.seekg (0, myfile.beg);

        if(length == 0)
            return;

        data.resize(length);

        myfile.read(data.data(), length);
    }
};

void test_serialisation();

#endif // SERIALISE_HPP_INCLUDED
