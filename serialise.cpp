#include "serialise.hpp"

uint64_t serialisable::gserialise_id;

std::map<int32_t, std::map<uint64_t, void*>> serialise_data_helper::owner_to_id_to_pointer;


void test_serialisation()
{
    float val = 0.f;

    serialise ser;
    ser.handle_serialise(val, true);
}
