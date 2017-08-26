#ifndef SERIALISE_HPP_INCLUDED
#define SERIALISE_HPP_INCLUDED

#include <new>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <map>

struct serialisable
{
    static uint64_t gserialise_id;
    uint64_t serialise_id = gserialise_id++;

    bool owned = true;
    int32_t owner_id = 0;

    bool handled_by_client = true;

    ///recieving
    static std::map<int32_t, std::map<uint64_t, void*>> owner_to_id_to_pointer;
};

template<typename T>
struct serialise_helper
{
    void add(T v, std::vector<char>& data)
    {
        char* pv = std::launder((char*)&v);

        for(uint32_t i=0; i<sizeof(T); i++)
        {
            data.push_back(pv[i]);
        }
    }

    T get(int& internal_counter, std::vector<char>& data)
    {
        int prev = internal_counter;

        internal_counter += sizeof(T);

        if(internal_counter > (int)data.size())
        {
            std::cout << "Error, invalid bytefetch" << std::endl;

            return T();
        }

        return *std::launder((T*)&data[prev]);
    }
};

template<typename T>
struct serialise_helper<T*>
{
    void add(T* v, std::vector<char>& data)
    {
        serialise_helper<decltype(serialisable::owner_id)> helper_owner_id;
        serialise_helper<decltype(serialisable::serialise_id)> helper1;

        helper_owner_id.add(v->owner_id, data);
        helper1.add(v->serialise_id, data);


        serialise_helper<T> helper2;
        helper2.add(*v, data);
    }

    ///ok. So. we're specialsied on a pointer which means we're requesting a pointer
    ///but in reality when we serialise, we're going to get our 64bit id followed by the data of the pointer

    T* get(int& internal_counter, std::vector<char>& data)
    {
        serialise_helper<decltype(serialisable::owner_id)> helper_owner_id;
        int32_t owner_id = helper_owner_id.get(internal_counter, data);

        serialise_helper<decltype(serialisable::serialise_id)> helper1;
        uint64_t id = helper1.get(internal_counter, data);

        //T* ptr = (T*)serialisable::id_to_pointer[id];

        T* ptr = (T*)serialisable::owner_to_id_to_pointer[owner_id][id];

        bool was_nullptr = ptr == nullptr;

        if(was_nullptr)
        {
            ptr = new T();
        }

        serialise_helper<T> data_fetcher;
        *ptr = data_fetcher.get(internal_counter, data);

        if(was_nullptr)
        {
            ptr->handled_by_client = false;
            ptr->owned = false;
            ptr->owner = owner_id;
        }

        return ptr;
    }
};

struct serialise
{
    std::vector<char> data;

    int internal_counter = 0;

    template<typename T>
    void push_back(T v)
    {
        serialise_helper<T> helper;

        helper.add(v, data);
    }

    ///if pointer, look up in pointer map
    template<typename T>
    T get()
    {
        serialise_helper<T> helper;

        return helper.get(internal_counter, data);
    }

    template<typename T>
    void handle_serialise(T& v, bool serialise)
    {
        if(serialise)
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
};


#endif // SERIALISE_HPP_INCLUDED
