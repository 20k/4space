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
};

void test_serialisation()
{
    {
        uint64_t val = 5343424;

        serialise ser;
        ser.handle_serialise(val, true);

        uint64_t rval;

        ser.handle_serialise(rval, false);

        assert(rval == val);
    }

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
}
