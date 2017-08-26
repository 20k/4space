#include "serialise.hpp"

#include <assert.h>

uint64_t serialisable::gserialise_id;

std::map<int32_t, std::map<uint64_t, void*>> serialise_data_helper::owner_to_id_to_pointer;

struct test_object : serialisable
{
    float v1 = 12.f;
    float v2 = 54.f;

    virtual void do_serialise(serialise& s, bool ser)
    {
        s.handle_serialise(v1, ser);
        s.handle_serialise(v2, ser);
    }

    virtual ~test_object(){}
};

void test_serialisation()
{
    ///test serialisation sanity for basic data types
    {
        uint64_t val = 5343424;

        serialise ser;
        ser.handle_serialise(val, true);

        uint64_t rval;

        ser.handle_serialise(rval, false);

        assert(rval == val);
    }

    ///test receiving new data
    {
        test_object* test = new test_object;

        serialise s2;

        s2.handle_serialise(test, true);


        ///emulate network receive
        serialise_data_helper::owner_to_id_to_pointer[test->owner_id][test->serialise_id] = nullptr;

        test_object* received;

        s2.handle_serialise(received, false);

        assert(received != nullptr);

        assert(received->handled_by_client == false);
        assert(received->owned == false);

        assert(received->v1 == test->v1);
        assert(received->v2 == test->v2);

        delete test;
        delete received;
    }

    ///test receiving data from/about ourselves to check ownership semantics work
    {
        test_object* test = new test_object;

        serialise s2;

        s2.handle_serialise(test, true);

        test_object* received;

        s2.handle_serialise(received, false);

        assert(received != nullptr);

        assert(received->handled_by_client == true);
        assert(received->owned == true);

        assert(received->v1 == test->v1);
        assert(received->v2 == test->v2);

        assert(test == received);

        delete test;
    }


    ///ok. Final test:
    ///Can we ping data from a to b, modify b, then ping it back to a
    {
        test_object* test = new test_object;

        serialise ser;
        ser.handle_serialise(test, true);

        serialise_data_helper::owner_to_id_to_pointer.clear();

        test_object* received;
        ser.handle_serialise(received, false);

        received->v1 = 99;
        received->v2 = 22;

        serialise s2;
        s2.handle_serialise(received, true);

        serialise_data_helper::owner_to_id_to_pointer[test->owner_id][test->serialise_id] = test;

        s2.handle_serialise(test, false);

        assert(test->v1 == received->v1);
        assert(test->v2 == received->v2);
    }
}
