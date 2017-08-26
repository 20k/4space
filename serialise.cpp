#include "serialise.hpp"

uint64_t serialisable::gserialise_id;

std::map<int32_t, std::map<uint64_t, void*>> serialise_data_helper::owner_to_id_to_pointer;


void test_serialisation()
{
    uint64_t val = 5343424;

    serialise ser;
    ser.handle_serialise(val, true);

    uint64_t rval;

    ser.handle_serialise(rval, false);

    std::cout << rval << std::endl;
}
